#include <iostream>
#include <string>
#include <algorithm>

// Ajusta rutas a tus headers
#include "./csvreader/CsvReader.h"
#include "./engine/SearchEngine.h"

// Tu función ya existe en utils.cpp (declárala aquí)
std::string normalizar_texto(const std::string& texto);

static bool starts_with(const std::string& s, const std::string& pref) {
    return s.size() >= pref.size() && s.compare(0, pref.size(), pref) == 0;
}

int main() {
    std::string path = "resources/mpst_full_data.csv"; // Cambia al path real del dataset

    std::cout << "Cargando CSV...\n";
    auto movies = CsvReader::cargar_csv(path);
    std::cout << "Peliculas cargadas: " << movies.size() << "\n";

    SearchEngine engine;
    engine.setMovies(std::move(movies));

    std::cout << "Construyendo indice trigram...\n";
    engine.buildTrigramIndex();
    std::cout << "Indice trigram listo.\n";

    std::cout << "Construyendo indice de tags...\n";
    engine.buildTagIndex();
    std::cout << "Indice de tags listo.\n";

    std::cout << "Construyendo indice de palabras...\n";
    engine.buildWordIndex();
    std::cout << "Indice de palabras listo.\n";

    while (true) {
        std::string q;
        std::cout << "\n==============================\n";
        std::cout << "Buscar (ENTER para salir)\n";
        std::cout << " - substring: barco | bar\n";
        std::cout << " - frase: barco fantasma\n";
        std::cout << " - tag: tag:horror  o  tag horror\n";
        std::cout << "> ";
        std::getline(std::cin, q);
        if (q.empty()) break;

        // -------- Detectar modo TAG --------
        bool isTagQuery = false;
        std::string tagRaw;

        if (starts_with(q, "tag:")) {
            isTagQuery = true;
            tagRaw = q.substr(4);
        } else if (starts_with(q, "tag ")) {
            isTagQuery = true;
            tagRaw = q.substr(4);
        }

        size_t offset = 0;

        // -------- Ejecutar búsqueda por TAG --------
        if (isTagQuery) {
            std::string tagNorm = normalizar_texto(tagRaw);

            while (true) {
                auto page = engine.searchByTag(tagNorm, offset, 5);

                if (page.empty()) {
                    if (offset == 0) std::cout << "Sin resultados para tag.\n";
                    break;
                }

                const auto& ms = engine.getMovies();
                for (size_t i = 0; i < page.size(); ++i) {
                    auto id = page[i].movieId;
                    std::cout << (offset + i + 1) << ") "
                              << ms[id].getTitle()
                              << "  [score=" << page[i].score << "]\n";
                }

                std::cout << "n) siguientes 5 | b) volver : ";
                std::string opt;
                std::getline(std::cin, opt);

                if (opt == "n") { offset += 5; continue; }
                break;
            }
            continue;
        }

        // -------- Búsqueda normal (substring o frase) --------
        std::string qNorm = normalizar_texto(q);
        bool isPhrase = (qNorm.find(' ') != std::string::npos);

        while (true) {
            auto page = isPhrase
                ? engine.searchPhrase(qNorm, offset, 5)
                : engine.searchSubstring(qNorm, offset, 5);

            if (page.empty()) {
                if (offset == 0) std::cout << "Sin resultados.\n";
                break;
            }

            const auto& ms = engine.getMovies();
            for (size_t i = 0; i < page.size(); ++i) {
                auto id = page[i].movieId;
                std::cout << (offset + i + 1) << ") "
                          << ms[id].getTitle()
                          << "  [score=" << page[i].score << "]\n";
            }

            std::cout << "n) siguientes 5 | b) volver : ";
            std::string opt;
            std::getline(std::cin, opt);

            if (opt == "n") { offset += 5; continue; }
            break;
        }
    }

    std::cout << "Saliendo...\n";
    return 0;
}