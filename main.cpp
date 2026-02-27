#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "movie/Movie.h"
#include "engine/SearchEngine.h"
#include "patterns/Observer.h"
#include "patterns/MovieDisplay.h"

std::string normalizar_texto(const std::string &texto);

static void limpiar_pantalla() {
    #ifdef _WIN32
        std::system("cls");
    #else
        std::system("clear");
    #endif
}

static void linea() {
    std::cout << "------------------------------------------------------------\n";
}

static std::string leer_linea(const std::string &prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

static int leer_int(const std::string &prompt, int minv, int maxv) {
    while (true) {
        std::string s = leer_linea(prompt);
        try {
            int v = std::stoi(s);
            if (v < minv || v > maxv) throw std::out_of_range("range");
            return v;
        } catch (...) {
            std::cout << "Entrada invalida. Intenta nuevamente.\n";
        }
    }
}

static bool leer_si_no(const std::string &prompt) {
    while (true) {
        std::string s = leer_linea(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        std::cout << "Respuesta invalida.\n";
    }
}

struct UserState {
    std::unordered_set<int> liked;
    std::unordered_set<int> watchLater;
    Subject* subject;
    SearchEngine* engine;

    const std::string likesFile = "likes.txt";
    const std::string watchLaterFile = "ver_mas_tarde.txt";

    UserState() : subject(nullptr), engine(nullptr) {}
    
    void setSubject(Subject* s) { subject = s; }
    void setEngine(SearchEngine* e) { engine = e; }

    void cargarDatos() {
        std::ifstream file(likesFile);
        int id;
        while (file >> id) {
            liked.insert(id);
        }
        file.close();

        std::ifstream file2(watchLaterFile);
        while (file2 >> id) {
            watchLater.insert(id);
        }
        file2.close();
    }

    void guardarDatos() {
        std::ofstream file(likesFile);
        for (int id : liked) {
            file << id << "\n";
        }
        file.close();

        std::ofstream file2(watchLaterFile);
        for (int id : watchLater) {
            file2 << id << "\n";
        }
        file2.close();
    }

    bool isLiked(int id) const { return liked.find(id) != liked.end(); }
    bool isWatchLater(int id) const { return watchLater.find(id) != watchLater.end(); }

    void toggleLike(int id) {
        std::string title = "";
        if (engine) {
            title = engine->getMovieById(id).getTitle();
        }
        
        if (isLiked(id)) {
            liked.erase(id);
            if (subject) subject->notifyMovieUnliked(id, title);
        } else {
            liked.insert(id);
            if (subject) subject->notifyMovieLiked(id, title);
        }
        guardarDatos();
    }

    void toggleWatchLater(int id) {
        std::string title = "";
        if (engine) {
            title = engine->getMovieById(id).getTitle();
        }
        
        if (isWatchLater(id)) {
            watchLater.erase(id);
            if (subject) subject->notifyMovieRemovedFromWatchlist(id, title);
        } else {
            watchLater.insert(id);
            if (subject) subject->notifyMovieAddedToWatchlist(id, title);
        }
        guardarDatos();
    }
};

const double USER_LIKE_BOOST = 12.0;

static void applyUserBoostAndSort(std::vector<SearchResult>& page,
                                 const UserState& user) {
    for (auto& r : page) {
        if (user.isLiked((int)r.movieId)) {
            r.score += USER_LIKE_BOOST;
        }
    }

    std::sort(page.begin(), page.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.movieId < b.movieId;
              });
}

static void printMovieCard(const Movie &m, int movieId, double score, const UserState &user) {
    std::cout << movieId << "  "
              << m.getTitle()
              << "  [score=" << score << "]";
    if (user.isLiked(movieId)) std::cout << "  <3";
    if (user.isWatchLater(movieId)) std::cout << "  [WL]";
    std::cout << "\n";
}

static void printMovieDetails(const Movie &m, int movieId, const UserState &user) {
    linea();
    std::cout << "ID: " << movieId << "\n";
    
    // Usar Decorator para mostrar detalles
    MovieDisplay* display = new ExtendedMovieDisplay(new BasicMovieDisplay());
    display->display(m);
    delete display;
    
    std::cout << "Liked: " << (user.isLiked(movieId) ? "SI" : "NO") << "\n";
    std::cout << "Ver mas tarde: " << (user.isWatchLater(movieId) ? "SI" : "NO") << "\n";
    linea();
}

static void showWatchLater(SearchEngine &engine, UserState &user) {
    limpiar_pantalla();
    linea();
    std::cout << "VER MAS TARDE (" << user.watchLater.size() << ")\n";
    linea();

    if (user.watchLater.empty()) {
        std::cout << "No tienes peliculas en Ver mas tarde.\n";
        leer_linea("Presiona Enter para volver...");
        return;
    }

    std::vector<int> ids(user.watchLater.begin(), user.watchLater.end());
    std::sort(ids.begin(), ids.end());

    for (int id : ids) {
        const Movie &m = engine.getMovieById(id);
        std::cout << id << "  " << m.getTitle()
                  << (user.isLiked(id) ? "  <3" : "")
                  << "\n";
    }

    if (!leer_si_no("Quieres abrir detalles de alguna?")) return;
    int movieId = leer_int("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        limpiar_pantalla();
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver mas tarde\n";
        std::cout << "  3) Volver\n";
        int a = leer_int("Elige accion: ", 1, 3);
        if (a == 3) break;
        if (a == 1) user.toggleLike(movieId);
        if (a == 2) user.toggleWatchLater(movieId);
    }
}

static void showLiked(SearchEngine &engine, UserState &user) {
    limpiar_pantalla();
    linea();
    std::cout << "PELICULAS LIKEADAS (" << user.liked.size() << ")\n";
    linea();

    if (user.liked.empty()) {
        std::cout << "No tienes peliculas likeadas.\n";
        leer_linea("Presiona Enter para volver...");
        return;
    }

    std::vector<int> ids(user.liked.begin(), user.liked.end());
    std::sort(ids.begin(), ids.end());

    for (int id : ids) {
        const Movie &m = engine.getMovieById(id);
        std::cout << id << "  " << m.getTitle()
                  << (user.isWatchLater(id) ? "  [WL]" : "")
                  << "\n";
    }

    if (!leer_si_no("Quieres abrir detalles de alguna?")) return;
    int movieId = leer_int("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        limpiar_pantalla();
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver mas tarde\n";
        std::cout << "  3) Volver\n";
        int a = leer_int("Elige accion: ", 1, 3);
        if (a == 3) break;
        if (a == 1) user.toggleLike(movieId);
        if (a == 2) user.toggleWatchLater(movieId);
    }
}

// ------------------ Flujo: ver resultados y elegir ------------------
static void browseResults(SearchEngine &engine,
                          const std::string &query_norm,
                          bool is_phrase,
                          UserState &user,
                          bool is_tag = false) {
    constexpr size_t PAGE = 5;
    size_t offset = 0;

    while (true) {
        std::vector<SearchResult> page;
        if (is_tag)
            page = engine.searchByTag(query_norm, offset, PAGE);
        else if (is_phrase)
            page = engine.searchPhrase(query_norm, offset, PAGE);
        else
            page = engine.searchSubstring(query_norm, offset, PAGE);

        limpiar_pantalla();
        linea();
        std::cout << "Resultados ("
                  << (is_tag ? "TAG" : is_phrase ? "FRASE" : "SUBSTRING")
                  << ") query=\"" << query_norm << "\""
                  << "  [offset=" << offset << "]\n";
        linea();

        if (page.empty()) {
            std::cout << "No hay mas resultados.\n";
            leer_linea("Presiona Enter para volver al menu...");
            return;
        } else {
            for (size_t i = 0; i < page.size(); ++i) {
                int id = page[i].movieId;
                const Movie &m = engine.getMovieById(id);
                printMovieCard(m, id, page[i].score, user);
            }
        }

        std::cout << "\nOpciones:\n";
        std::cout << "  1) Ver detalles (por ID)\n";
        std::cout << "  2) Siguiente pagina\n";
        std::cout << "  3) Pagina anterior\n";
        std::cout << "  4) Salir a menu\n";

        int op = leer_int("Elige opcion: ", 1, 4);

        if (op == 4) return;
        if (op == 2) { offset += PAGE; continue; }
        if (op == 3) { offset = (offset >= PAGE) ? (offset - PAGE) : 0; continue; }

        int movieId = leer_int("Ingresa movieId: ", 0, 2000000000);
        const Movie &m = engine.getMovieById(movieId);

        while (true) {
            limpiar_pantalla();
            printMovieDetails(m, movieId, user);
            std::cout << "Acciones:\n";
            std::cout << "  1) Toggle Like (reordena resultados)\n";
            std::cout << "  2) Toggle Ver mas tarde\n";
            std::cout << "  3) Volver a resultados\n";

            int a = leer_int("Elige accion: ", 1, 3);
            if (a == 3) break;

            if (a == 1) {
                user.toggleLike(movieId);
                break;
            } else if (a == 2) {
                user.toggleWatchLater(movieId);
            }
        }
    }
}

int main() {
    SearchEngine* engine = SearchEngine::getInstance();
    
    Subject subject;
    ConsoleLogger logger;
    subject.attach(&logger);
    
    UserState user;
    user.setSubject(&subject);
    user.setEngine(engine);
    
    user.cargarDatos();

    std::string path = "../resources/mpst_full_data.csv";

    // Usar un thread para cargar el CSV y construir indices
    std::cout << "Cargando datos..." << std::endl;
    std::thread cargaThread([&engine, &path]() {
        engine->load(path);
        engine->buildIndexes();
    });
    
    cargaThread.join();
    
    std::cout << "Listo! Total de peliculas: " << engine->movieCount() << "\n";

    if (!user.watchLater.empty()) {
        std::cout << "\n=== PELICULAS EN VER MAS TARDE ===\n";
        showWatchLater(*engine, user);
    }

    while (true) {
        limpiar_pantalla();
        linea();
        std::cout << "Bienvenido a Rotten Potatoes. Elige una opción\n";
        linea();
        std::cout << "1) Buscar pelicula por palabra\n";
        std::cout << "2) Buscar pelicula por frase\n";
        std::cout << "3) Buscar por etiqueta (tag)\n";
        std::cout << "4) Mi lista de Ver mas tarde\n";
        std::cout << "5) Ver peliculas likeadas\n";
        std::cout << "6) Salir\n";

        int op = leer_int("Escribe tu opcion: ", 1, 6);
        if (op == 6) break;

        if (op == 4) { showWatchLater(*engine, user); continue; }
        if (op == 5) { showLiked(*engine, user); continue; }

        std::string q = leer_linea("Query: ");
        q = normalizar_texto(q);

        if (q.empty()) {
            std::cout << "Query vacia.\n";
            leer_linea("Presiona Enter para continuar...");
            continue;
        }

        if (op == 3) {
            browseResults(*engine, q, false, user, true);
        } else {
            bool is_phrase = (op == 2);
            browseResults(*engine, q, is_phrase, user, false);
        }
    }

    user.guardarDatos();
    std::cout << "Vuelve pronto :D.\n";
    return 0;
}