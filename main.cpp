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
    // std::cout << "Nodos en trie: " << engine.getTrigramNodeCount() << "\n";
    std::cout << "Indice listo.\n";

    while (true) {
        std::string q;
        std::cout << "\nBuscar (ENTER para salir): ";
        std::getline(std::cin, q);
        if (q.empty()) break;

        //std::string qn = normalizar_texto(q);
        std::string qn = q;

        size_t offset = 0;
        while (true) {
            auto page = engine.searchSubstring(qn, offset, 5);

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

            std::cout << "n) siguientes 5 | b) volver | elegir numero para ver sinopsis: ";
            std::string opt;
            std::getline(std::cin, opt);

            if (opt == "n") { offset += 5; continue; }
            if (opt == "b") break;

            // elegir numero global
            try {
                int k = std::stoi(opt);
                int idxGlobal = k - 1;
                if (idxGlobal < 0) continue;

                // Para simplificar: recalcular la misma página donde cae (solo demo).
                // Mejor: guarda el vector total o un cache por query.
                size_t pageOffset = (size_t)(idxGlobal / 5) * 5;
                auto pg = engine.searchSubstring(qn, pageOffset, 5);
                int local = idxGlobal - (int)pageOffset;
                if (local < 0 || local >= (int)pg.size()) continue;

                auto mid = pg[local].movieId;
                std::cout << "\n=== " << ms[mid].getTitle() << " ===\n";
                std::cout << ms[mid].getPlot() << "\n";
                std::cout << "1) Like  2) Ver mas tarde  3) Volver : ";
                std::string a;
                std::getline(std::cin, a);
                if (a == "1") engine.getMovies()[mid].like();
                else if (a == "2") engine.getMovies()[mid].setWatchLater(true);
            } catch (...) {
                // ignore
            }
        }
    }

    std::cout << "Saliendo...\n";
    return 0;
}