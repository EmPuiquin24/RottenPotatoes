#include <iostream>
#include <string>
#include "./csvreader/CsvReader.h"
#include "./engine/SearchEngine.h"

// Tu normalizar_texto ya existe en utils; úsala para normalizar el query
//std::string normalizar_texto(const std::string& texto);

int main() {
    std::string path = "resources/mpst_full_data.csv";

    std::cout << "Cargando CSV...\n";
    auto movies = CsvReader::cargar_csv(path);

    std::cout << "Peliculas cargadas: " << movies.size() << "\n";

    SearchEngine engine;
    engine.setMovies(std::move(movies));

    std::cout << "Construyendo indice trigram...\n";
    engine.buildTrigramIndex();
    std::cout << "Indice listo.\n";

    std::cout << "Construyendo indice de tags...\n";   // <-- AGREGA ESTO
    engine.buildTagIndex();                             // <-- AGREGA ESTO
    std::cout << "Indice de tags listo.\n";             // <-- AGREGA ESTO


    while (true) {
        std::string q;
        std::cout << "\nBuscar (ENTER para salir).\n";
        std::cout << " - substring: barco\n";
        std::cout << " - tag: tag:horror  o  tag horror\n";
        std::cout << "> ";
        std::getline(std::cin, q);
        if (q.empty())
            break;

        // Detectar modo tag
        bool isTagQuery = false;
        std::string tag;

        if (q.rfind("tag:", 0) == 0) {
            // empieza con "tag:"
            isTagQuery = true;
            tag = q.substr(4);
        } else if (q.rfind("tag ", 0) == 0) {
            // empieza con "tag "
            isTagQuery = true;
            tag = q.substr(4);
        }

        size_t offset = 0;

        if (isTagQuery) {
            std::string tag_norm = normalizar_texto(tag);

            while (true) {
                auto page = engine.searchByTag(tag_norm, offset, 5);

                if (page.empty()) {
                    if (offset == 0)
                        std::cout << "Sin resultados para tag.\n";
                    break;
                }

                const auto &ms = engine.getMovies();
                for (size_t i = 0; i < page.size(); ++i) {
                    auto id = page[i].movieId;
                    std::cout << (offset + i + 1) << ") " << ms[id].getTitle()
                            << "  [score=" << page[i].score << "]\n";
                }

                std::cout << "n) siguientes 5 | b) volver : ";
                std::string opt;
                std::getline(std::cin, opt);
                if (opt == "n") {
                    offset += 5;
                    continue;
                }
                break;
            }

            continue; // vuelve al inicio del loop
        }

        // Caso substring normal (lo que ya tenías)
        std::string qn = normalizar_texto(q);
        while (true) {
            auto page = engine.searchSubstring(qn, offset, 5);

            if (page.empty()) {
                if (offset == 0)
                    std::cout << "Sin resultados.\n";
                break;
            }

            const auto &ms = engine.getMovies();
            for (size_t i = 0; i < page.size(); ++i) {
                auto id = page[i].movieId;
                std::cout << (offset + i + 1) << ") " << ms[id].getTitle()
                        << "  [score=" << page[i].score << "]\n";
            }

            std::cout << "n) siguientes 5 | b) volver : ";
            std::string opt;
            std::getline(std::cin, opt);
            if (opt == "n") {
                offset += 5;
                continue;
            }
            break;
        }
    }

    std::cout << "Saliendo...\n";
    return 0;
}
