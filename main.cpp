#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <sstream>

#include "movie/Movie.h"
#include "engine/SearchEngine.h"

// Si tus utils están en otro lado:
std::string normalizar_texto(const std::string &texto);

// ------------------ Helpers UI ------------------

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
            std::cout << "Entrada inválida. Intenta nuevamente.\n";
        }
    }
}

static bool readYesNo(const std::string &prompt) {
    while (true) {
        std::string s = readLine(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        std::cout << "Respuesta inválida.\n";
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

    bool isLiked(int id) const { return liked.find(id) != liked.end(); }
    bool isWatchLater(int id) const { return watchLater.find(id) != watchLater.end(); }

    void toggleLike(int id) {
        if (isLiked(id)) liked.erase(id);
        else liked.insert(id);
    }

    void toggleWatchLater(int id) {
        if (isWatchLater(id)) watchLater.erase(id);
        else watchLater.insert(id);
    }
};

// ------------------ Boost de usuario en ranking (Modelo A) ------------------
// Ajusta si quieres que el like tenga más/menos impacto.
constexpr double USER_LIKE_BOOST = 12.0;

// Aplica boost SOLO a la página actual y reordena esa página (simple y efectivo).
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
              << (user.isLiked(movieId) ? "  ❤️" : "")
              << (user.isWatchLater(movieId) ? "  ⏰" : "")
              << "\n";
}

static void printMovieDetails(const Movie &m, int movieId, const UserState &user) {
    clearLine();
    std::cout << "ID: " << movieId << "\n";
    std::cout << "Título: " << m.getTitle() << "\n";
    std::cout << "Liked: " << (user.isLiked(movieId) ? "SI" : "NO") << "\n";
    std::cout << "Ver más tarde: " << (user.isWatchLater(movieId) ? "SI" : "NO") << "\n";

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

    if (readYesNo("¿Mostrar sinopsis completa?")) {
        std::cout << m.getPlot() << "\n";
        clearLine();
    }
}

// ------------------ Watch Later Home ------------------
static void showWatchLater(SearchEngine &engine, UserState &user) {
    clearLine();
    std::cout << "VER MAS TARDE (" << user.watchLater.size() << ")\n";
    clearLine();

    if (user.watchLater.empty()) {
        std::cout << "No tienes películas en Ver más tarde.\n";
        return;
    }

    std::vector<int> ids(user.watchLater.begin(), user.watchLater.end());
    std::sort(ids.begin(), ids.end());

    for (int id : ids) {
        const Movie &m = engine.getMovieById(id);
        std::cout << id << "  " << m.getTitle()
                  << (user.isLiked(id) ? "  ❤️" : "")
                  << "\n";
    }

    if (!readYesNo("¿Quieres abrir detalles de alguna?")) return;
    int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver más tarde\n";
        std::cout << "  3) Volver\n";
        int a = readInt("Elige acción: ", 1, 3);
        if (a == 3) break;
        if (a == 1) user.toggleLike(movieId);
        if (a == 2) user.toggleWatchLater(movieId);
    }
}

// ------------------ Liked Home ------------------
static void showLiked(SearchEngine &engine, UserState &user) {
    clearLine();
    std::cout << "PELÍCULAS LIKEADAS (" << user.liked.size() << ")\n";
    clearLine();

    if (user.liked.empty()) {
        std::cout << "No tienes películas likeadas.\n";
        return;
    }

    std::vector<int> ids(user.liked.begin(), user.liked.end());
    std::sort(ids.begin(), ids.end());

    for (int id : ids) {
        const Movie &m = engine.getMovieById(id);
        std::cout << id << "  " << m.getTitle()
                  << (user.isWatchLater(id) ? "  ⏰" : "")
                  << "\n";
    }

    if (!readYesNo("¿Quieres abrir detalles de alguna?")) return;
    int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
    const Movie &m = engine.getMovieById(movieId);

    while (true) {
        printMovieDetails(m, movieId, user);
        std::cout << "Acciones:\n";
        std::cout << "  1) Toggle Like\n";
        std::cout << "  2) Toggle Ver más tarde\n";
        std::cout << "  3) Volver\n";
        int a = readInt("Elige acción: ", 1, 3);
        if (a == 3) break;
        if (a == 1) user.toggleLike(movieId);
        if (a == 2) user.toggleWatchLater(movieId);
    }
}

// ------------------ Flujo: ver resultados y elegir ------------------
static void browseResults(SearchEngine &engine,
                          const std::string &query_norm,
                          bool is_phrase,
                          UserState &user) {
    constexpr size_t PAGE = 5;
    size_t offset = 0;

    while (true) {
        std::vector<SearchResult> page = is_phrase
            ? engine.searchPhrase(query_norm, offset, PAGE)
            : engine.searchSubstring(query_norm, offset, PAGE);

        // Modelo A: boost por like del usuario + ordenar SOLO esta página
        applyUserBoostAndSort(page, user);

        clearLine();
        std::cout << "Resultados (" << (is_phrase ? "FRASE" : "SUBSTRING")
                  << ") query=\"" << query_norm << "\""
                  << "  [offset=" << offset << "]\n";
        clearLine();

        if (page.empty()) {
            std::cout << "No hay más resultados.\n";
        } else {
            for (size_t i = 0; i < page.size(); ++i) {
                int id = page[i].movieId;
                const Movie &m = engine.getMovieById(id);
                printMovieCard(m, id, page[i].score, user);
            }
        }

        std::cout << "\nOpciones:\n";
        std::cout << "  1) Ver detalles (por ID)\n";
        std::cout << "  2) Siguiente página\n";
        std::cout << "  3) Página anterior\n";
        std::cout << "  4) Salir a menú\n";

        int op = readInt("Elige opción: ", 1, 4);

        if (op == 4) return;
        if (op == 2) { offset += PAGE; continue; }
        if (op == 3) { offset = (offset >= PAGE) ? (offset - PAGE) : 0; continue; }

        int movieId = readInt("Ingresa movieId: ", 0, 2000000000);
        const Movie &m = engine.getMovieById(movieId);

        while (true) {
            printMovieDetails(m, movieId, user);
            std::cout << "Acciones:\n";
            std::cout << "  1) Toggle Like (reordena resultados)\n";
            std::cout << "  2) Toggle Ver más tarde\n";
            std::cout << "  3) Volver a resultados\n";

            int a = readInt("Elige acción: ", 1, 3);
            if (a == 3) break;

            if (a == 1) {
                user.toggleLike(movieId);
                // volvemos a resultados para recalcular y ver el reordenamiento
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

    SearchEngine engine;
    UserState user;

    // Path por defecto
    std::string path = "resources/mpst_full_data.csv";

    std::cout << "[MAIN] Cargando CSV: " << path << std::endl;
    engine.load(path);
    std::cout << "[MAIN] CSV cargado. Construyendo índices..." << std::endl;
    engine.buildIndexes();
    std::cout << "[MAIN] Índices listos.\n";

    while (true) {
        clearLine();
        std::cout << "PLATAFORMA DE STREAMING (Consola)\n";
        clearLine();
        std::cout << "1) Buscar (palabra / substring)\n";
        std::cout << "2) Buscar (frase)\n";
        std::cout << "3) Ver 'Ver más tarde'\n";
        std::cout << "4) Ver películas likeadas\n";
        std::cout << "5) Salir\n";

        int op = readInt("Elige opción: ", 1, 5);
        if (op == 5) break;

        if (op == 3) { showWatchLater(engine, user); continue; }
        if (op == 4) { showLiked(engine, user); continue; }

        std::string q = readLine("Query: ");
        q = normalizar_texto(q);

        if (q.empty()) {
            std::cout << "Query vacía.\n";
            continue;
        }

        bool is_phrase = (op == 2);
        browseResults(engine, q, is_phrase, user);
    }

    std::cout << "Bye.\n";
    return 0;
}