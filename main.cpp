#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <cstdlib>

#include "movie/Movie.h"
#include "engine/SearchEngine.h"
#include "patterns/MovieObserver.h"
#include "patterns/QueryBuilder.h"

// Si tus utils estan en otro lado:
std::string normalizar_texto(const std::string &texto);

// ------------------ Helpers UI ------------------

static void clear_screen() {
    #ifdef _WIN32
        std::system("cls"); // For Windows
    #else
        std::system("clear"); // For Linux/Unix/macOS
    #endif
}

static void clearLine() {
    std::cout << "------------------------------------------------------------\n";
}

static std::string readLine(const std::string &prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

static int readInt(const std::string &prompt, int minv, int maxv) {
    while (true) {
        std::string s = readLine(prompt);
        try {
            int v = std::stoi(s);
            if (v < minv || v > maxv) throw std::out_of_range("range");
            return v;
        } catch (...) {
            std::cout << "Entrada invalida. Intenta nuevamente.\n";
        }
    }
}

static bool readYesNo(const std::string &prompt) {
    while (true) {
        std::string s = readLine(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        std::cout << "Respuesta invalida.\n";
    }
}

// Primeras N palabras (para no imprimir toda la sinopsis)
static std::string firstNWords(const std::string& text, size_t n) {
    std::istringstream iss(text);
    std::ostringstream oss;

    std::string w;
    size_t count = 0;

    while (iss >> w) {
        if (count > 0) oss << ' ';
        oss << w;
        ++count;
        if (count >= n) break;
    }
    if (iss >> w) oss << " ...";
    return oss.str();
}

// ------------------ Estado del usuario ------------------
struct UserState {
    std::unordered_set<int> liked;       // movieId
    std::unordered_set<int> watchLater;  // movieId
    MovieSubject* subject;               // Patron Observer
    SearchEngine* engine;                // Referencia al motor de busqueda

    UserState() : subject(nullptr), engine(nullptr) {}
    
    void setSubject(MovieSubject* s) { subject = s; }
    void setEngine(SearchEngine* e) { engine = e; }

    bool isLiked(int id) const { return liked.find(id) != liked.end(); }
    bool isWatchLater(int id) const { return watchLater.find(id) != watchLater.end(); }

    void toggleLike(int id) {
        std::string title = engine ? engine->getMovieById(id).getTitle() : "";
        
        if (isLiked(id)) {
            liked.erase(id);
            if (subject) subject->notifyMovieUnliked(id, title);
        } else {
            liked.insert(id);
            if (subject) subject->notifyMovieLiked(id, title);
        }
    }

    void toggleWatchLater(int id) {
        std::string title = engine ? engine->getMovieById(id).getTitle() : "";
        
        if (isWatchLater(id)) {
            watchLater.erase(id);
            if (subject) subject->notifyMovieRemovedFromWatchlist(id, title);
        } else {
            watchLater.insert(id);
            if (subject) subject->notifyMovieAddedToWatchlist(id, title);
        }
    }
};

// ------------------ Boost de usuario en ranking (Modelo A) ------------------
// Ajusta si quieres que el like tenga mas/menos impacto.
constexpr double USER_LIKE_BOOST = 12.0;

// Aplica boost SOLO a la pagina actual y reordena esa pagina (simple y efectivo).
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

// ------------------ Render de película ------------------
static void printMovieCard(const Movie &m, int movieId, double score, const UserState &user) {
    std::cout << std::setw(6) << movieId << "  "
              << m.getTitle()
              << "  [score=" << score << "]"
              << (user.isLiked(movieId) ? "  <3" : "")
              << (user.isWatchLater(movieId) ? "  [WL]" : "")
              << "\n";
}

static void printMovieDetails(const Movie &m, int movieId, const UserState &user) {
    clearLine();
    std::cout << "ID: " << movieId << "\n";
    std::cout << "Titulo: " << m.getTitle() << "\n";
    std::cout << "Liked: " << (user.isLiked(movieId) ? "SI" : "NO") << "\n";
    std::cout << "Ver mas tarde: " << (user.isWatchLater(movieId) ? "SI" : "NO") << "\n";

    const auto &tags = m.getTags();
    if (!tags.empty()) {
        std::cout << "Tags: ";
        for (size_t i = 0; i < tags.size(); ++i) {
            std::cout << tags[i];
            if (i + 1 < tags.size()) std::cout << ", ";
        }
        std::cout << "\n";
    }

    clearLine();
    std::cout << "SINOPSIS (primeras 100 palabras):\n";
    std::cout << firstNWords(m.getPlot(), 100) << "\n";
    clearLine();

    if (readYesNo("Mostrar sinopsis completa?")) {
        std::cout << m.getPlot() << "\n";
        clearLine();
    }
}

// ------------------ Watch Later Home ------------------
static void showWatchLater(SearchEngine &engine, UserState &user) {
    clear_screen();
    clearLine();
    std::cout << "VER MAS TARDE (" << user.watchLater.size() << ")\n";
    clearLine();

    if (user.watchLater.empty()) {
        std::cout << "No tienes peliculas en Ver mas tarde.\n";
        readLine("Presiona Enter para volver...");
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

    if (!readYesNo("Quieres abrir detalles de alguna?")) return;
    int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        clear_screen();
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver mas tarde\n";
        std::cout << "  3) Volver\n";
        int a = readInt("Elige accion: ", 1, 3);
        if (a == 3) break;
        if (a == 1) user.toggleLike(movieId);
        if (a == 2) user.toggleWatchLater(movieId);
    }
}

// ------------------ Liked Home ------------------
static void showLiked(SearchEngine &engine, UserState &user) {
    clear_screen();
    clearLine();
    std::cout << "PELICULAS LIKEADAS (" << user.liked.size() << ")\n";
    clearLine();

    if (user.liked.empty()) {
        std::cout << "No tienes peliculas likeadas.\n";
        readLine("Presiona Enter para volver...");
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

    if (!readYesNo("Quieres abrir detalles de alguna?")) return;
    int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        clear_screen();
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver mas tarde\n";
        std::cout << "  3) Volver\n";
        int a = readInt("Elige accion: ", 1, 3);
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

        applyUserBoostAndSort(page, user);

        clear_screen();
        clearLine();
        std::cout << "Resultados ("
                  << (is_tag ? "TAG" : is_phrase ? "FRASE" : "SUBSTRING")
                  << ") query=\"" << query_norm << "\""
                  << "  [offset=" << offset << "]\n";
        clearLine();

        if (page.empty()) {
            std::cout << "No hay mas resultados.\n";
            readLine("Presiona Enter para volver al menu...");
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

        int op = readInt("Elige opcion: ", 1, 4);

        if (op == 4) return;
        if (op == 2) { offset += PAGE; continue; }
        if (op == 3) { offset = (offset >= PAGE) ? (offset - PAGE) : 0; continue; }

        int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
        const Movie &m = engine.getMovieById(movieId);

        while (true) {
            clear_screen();
            printMovieDetails(m, movieId, user);
            std::cout << "Acciones:\n";
            std::cout << "  1) Toggle Like (reordena resultados)\n";
            std::cout << "  2) Toggle Ver mas tarde\n";
            std::cout << "  3) Volver a resultados\n";

            int a = readInt("Elige accion: ", 1, 3);
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

// ------------------ main ------------------
int main() {
    std::cout.setf(std::ios::fixed);
    std::cout << std::showpoint << std::setprecision(3);

    // Usar patron Singleton para SearchEngine
    SearchEngine* engine = SearchEngine::getInstance();
    
    // Configurar Patron Observer
    MovieSubject subject;
    ConsoleLogger logger;
    StatisticsTracker stats;
    
    subject.attach(&logger);
    subject.attach(&stats);
    
    UserState user;
    user.setSubject(&subject);
    user.setEngine(engine);

    // Path por defecto
    // std::string path = "../resources/mpst_full_data.csv";
    std::string path = "resources/mpst_full_data.csv";

    std::cout << "[MAIN] Cargando CSV: " << path << std::endl;
    engine->load(path);
    
    std::cout << "[MAIN] CSV cargado. Construyendo indices con programacion paralela..." << std::endl;

    // Usar version paralela para mejor performance
    auto start = std::chrono::high_resolution_clock::now();
    engine->buildIndexesParallel();
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "[MAIN] Indices construidos en " << elapsed.count() << " segundos.\n";
    std::cout << "[MAIN] Total de peliculas: " << engine->movieCount() << "\n";

    while (true) {
        clear_screen();
        clearLine();
        std::cout << "PLATAFORMA DE STREAMING (Consola)\n";
        clearLine();
        std::cout << "1) Buscar (palabra / substring)\n";
        std::cout << "2) Buscar (frase)\n";
        std::cout << "3) Buscar por tag\n";
        std::cout << "4) Ver mas tarde\n";
        std::cout << "5) Ver peliculas likeadas\n";
        std::cout << "6) Ver estadisticas\n";
        std::cout << "7) Salir\n";

        int op = readInt("Elige opcion: ", 1, 7);
        if (op == 7) break;

        if (op == 4) { showWatchLater(*engine, user); continue; }
        if (op == 5) { showLiked(*engine, user); continue; }
        if (op == 6) {
            clear_screen();
            clearLine();
            stats.printStatistics();
            std::cout << "Peliculas likeadas: " << user.liked.size() << "\n";
            std::cout << "Peliculas en Ver mas tarde: " << user.watchLater.size() << "\n";
            clearLine();
            readLine("Presiona Enter para volver...");
            continue;
        }

        std::string q = readLine("Query: ");
        q = normalizar_texto(q);

        if (q.empty()) {
            std::cout << "Query vacia.\n";
            readLine("Presiona Enter para continuar...");
            continue;
        }

        if (op == 3) {
            browseResults(*engine, q, false, user, true);
        } else {
            bool is_phrase = (op == 2);
            browseResults(*engine, q, is_phrase, user, false);
        }
    }

    std::cout << "Bye.\n";
    return 0;
}