#include "SearchEngine.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

void SearchEngine::buildTrigramIndex() {
    // Construye trigramas (n=3) sobre full_text
    for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
        const std::string &text = movies[i].getFullText();
        if (text.size() < 3) continue;

        // Inserta cada trigrama
        for (size_t j = 0; j + 3 <= text.size(); ++j) {
            // Evita trigramas con espacios raros si quieres, aquí lo dejamos simple
            trigramTrie.insert(text.substr(j, 3), i);
        }
    }
    trigramTrie.finalize();
}

std::vector<SearchEngine::MovieId>
SearchEngine::intersectSorted(const std::vector<MovieId> &a,
                              const std::vector<MovieId> &b) {
    std::vector<MovieId> out;
    out.reserve(std::min(a.size(), b.size()));
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            out.push_back(a[i]);
            ++i;
            ++j;
        } else if (a[i] < b[j]) ++i;
        else ++j;
    }
    return out;
}

double SearchEngine::scoreMovie(const Movie &m, const std::string &q) {
    // Ranking simple y defendible
    // +10 si aparece en title, +3 si aparece en plot, +log(1+likes)
    double s = 0.0;
    if (m.getTitle().find(q) != std::string::npos) s += 10.0;
    if (m.getPlot().find(q) != std::string::npos) s += 3.0;
    s += std::log(1.0 + (double) m.getLikes());
    return s;
}

std::vector<SearchResult>
SearchEngine::searchSubstring(const std::string &q_norm,
                              size_t offset,
                              size_t limit) const {
    std::vector<SearchResult> empty;
    if (q_norm.empty()) return empty;

    // Queries cortas: por ahora fallback a scan directo (para que funcione ya)
    // Luego lo mejoramos con word index.
    if (q_norm.size() < 3) {
        std::vector<SearchResult> all;
        all.reserve(256);
        for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
            if (movies[i].getFullText().find(q_norm) != std::string::npos) {
                all.push_back({i, scoreMovie(movies[i], q_norm)});
            }
        }
        std::sort(all.begin(), all.end(), [](auto &x, auto &y) {
            if (x.score != y.score) return x.score > y.score;
            return x.movieId < y.movieId;
        });
        if (offset >= all.size()) return empty;
        size_t end = std::min(all.size(), offset + limit);
        return std::vector<SearchResult>(all.begin() + offset, all.begin() + end);
    }

    // 1) candidatos por trigramas (intersección)
    std::vector<MovieId> cand;
    bool first = true;

    for (size_t i = 0; i + 3 <= q_norm.size(); ++i) {
        std::string gram = q_norm.substr(i, 3);
        const auto *p = trigramTrie.getPostings(gram);
        if (!p) return empty;

        if (first) {
            cand = *p;
            first = false;
        } else {
            cand = intersectSorted(cand, *p);
        }
        if (cand.empty()) return empty;
    }

    // 2) verificación exacta + ranking
    std::vector<SearchResult> results;
    results.reserve(std::min<size_t>(cand.size(), 4096));

    for (MovieId id: cand) {
        const Movie &m = movies[id];
        if (m.getFullText().find(q_norm) != std::string::npos) {
            results.push_back({id, scoreMovie(m, q_norm)});
        }
    }

    // 3) ordenar por score desc y paginar
    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.movieId < b.movieId;
    });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}

// void SearchEngine::buildTagIndex() {
//     tagIndex.clear();
//     tagIndex.reserve(movies.size() / 4); // heurística ligera
//
//     for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
//         for (const auto &tag: movies[i].getTags()) {
//             if (!tag.empty()) tagIndex[tag].push_back(i);
//         }
//     }
//
//     // sort+unique por cada tag para intersecciones/orden y evitar repetidos
//     for (auto &kv: tagIndex) {
//         sortUnique(kv.second);
//     }
// }

void SearchEngine::buildTagIndex() {
    tagIndex.clear();
    tagIndex.reserve(movies.size() / 4);

    for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
        for (const auto &tag: movies[i].getTags()) {
            if (tag.empty()) continue;

            // Normalizar el tag igual que se normaliza la query
            std::string tag_norm = tag;
            std::transform(tag_norm.begin(), tag_norm.end(), tag_norm.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            // Trim espacios al inicio y al final
            size_t start = tag_norm.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            size_t end = tag_norm.find_last_not_of(" \t\r\n");
            tag_norm = tag_norm.substr(start, end - start + 1);

            if (!tag_norm.empty()) tagIndex[tag_norm].push_back(i);
        }
    }

    for (auto &kv: tagIndex) {
        sortUnique(kv.second);
    }
}


std::vector<SearchResult>
SearchEngine::searchByTag(const std::string &tag_norm,
                          size_t offset,
                          size_t limit) const {
    std::vector<SearchResult> empty;
    if (tag_norm.empty()) return empty;

    auto it = tagIndex.find(tag_norm);
    if (it == tagIndex.end()) return empty;

    const auto &ids = it->second;

    std::vector<SearchResult> results;
    results.reserve(ids.size());

    // Ranking simple: likes como importancia base (puedes enriquecer luego)
    for (MovieId id: ids) {
        const Movie &m = movies[id];
        double s = std::log(1.0 + (double) m.getLikes());
        results.push_back({id, s});
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.movieId < b.movieId;
              });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}


// Tokeniza por espacios (query_norm ya está limpio)
static std::vector<std::string> tokenize_ws(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        if (!tok.empty()) out.push_back(tok);
    }
    return out;
}

void SearchEngine::buildWordIndex() {
    wordIndex.clear();
    wordIndex.reserve(movies.size()); // heurística

    for (MovieId i = 0; i < (MovieId)movies.size(); ++i) {
        // Indexamos palabras de título + sinopsis (ya normalizados en Movie)
        const std::string& text = movies[i].getFullText();
        auto tokens = tokenize_ws(text);

        for (const auto& w : tokens) {
            if (!w.empty()) wordIndex[w].push_back(i);
        }
    }

    // sort+unique por palabra para poder unir/intersectar rápido
    for (auto& kv : wordIndex) {
        sortUnique(kv.second);
    }
}

std::vector<SearchEngine::MovieId>
SearchEngine::unionSorted(const std::vector<MovieId>& a,
                          const std::vector<MovieId>& b) {
    std::vector<MovieId> out;
    out.reserve(a.size() + b.size());
    size_t i=0, j=0;
    while (i<a.size() && j<b.size()) {
        if (a[i]==b[j]) { out.push_back(a[i]); ++i; ++j; }
        else if (a[i]<b[j]) { out.push_back(a[i]); ++i; }
        else { out.push_back(b[j]); ++j; }
    }
    while (i<a.size()) out.push_back(a[i++]);
    while (j<b.size()) out.push_back(b[j++]);

    // out ya queda ordenado; puede tener repetidos si a/b no eran unique (pero lo son)
    // igual lo dejamos robusto:
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

int SearchEngine::countOccurrences(const std::string& text, const std::string& pattern) {
    if (pattern.empty()) return 0;
    int cnt = 0;
    size_t pos = 0;
    while (true) {
        pos = text.find(pattern, pos);
        if (pos == std::string::npos) break;
        ++cnt;
        pos += pattern.size(); // no overlaps (ok para ranking)
    }
    return cnt;
}

std::vector<SearchResult>
SearchEngine::searchPhrase(const std::string& query_norm,
                           size_t offset,
                           size_t limit) const {
    std::vector<SearchResult> empty;
    if (query_norm.empty()) return empty;

    // 1) tokens
    auto tokens = tokenize_ws(query_norm);
    if (tokens.empty()) return empty;

    // 2) obtener postings por token (si un token no existe, su postings es vacío)
    std::vector<MovieId> cand_or;
    bool first_or = true;

    // Para AND: empezamos con postings del primer token existente.
    std::vector<MovieId> cand_and;
    bool and_initialized = false;

    for (const auto& t : tokens) {
        auto it = wordIndex.find(t);
        if (it == wordIndex.end()) {
            // token no existe => para AND, hace imposible cumplir todos
            continue;
        }
        const auto& p = it->second;

        // OR
        if (first_or) { cand_or = p; first_or = false; }
        else { cand_or = unionSorted(cand_or, p); }

        // AND
        if (!and_initialized) { cand_and = p; and_initialized = true; }
        else { cand_and = intersectSorted(cand_and, p); }
    }

    if (first_or) {
        // Ningún token existía
        return empty;
    }

    // 3) Ranking de candidatos OR, con bonus si está en AND, bonus si frase exacta
    std::vector<SearchResult> results;
    results.reserve(std::min<size_t>(cand_or.size(), 10000));

    // Para chequear rápido si id pertenece a cand_and:
    // cand_and está ordenado => usamos binary_search
    auto in_and = [&](MovieId id) {
        if (!and_initialized) return false;
        return std::binary_search(cand_and.begin(), cand_and.end(), id);
    };

    for (MovieId id : cand_or) {
        const Movie& m = movies[id];
        const std::string& title = m.getTitle();
        const std::string& plot  = m.getPlot();
        const std::string& full  = m.getFullText();

        double s = 0.0;

        // Score por ocurrencias de tokens (y/o)
        for (const auto& t : tokens) {
            if (t.empty()) continue;

            // pesos diferenciados: título pesa más
            s += 10.0 * (double)countOccurrences(title, t);
            s += 3.0  * (double)countOccurrences(plot, t);
        }

        // Bonus por cumplir todas las palabras (AND)
        if (in_and(id)) s += 25.0;

        // Bonus por contener la frase completa exacta
        if (full.find(query_norm) != std::string::npos) s += 40.0;

        // Boost por likes
        s += std::log(1.0 + (double)m.getLikes());

        // Si por alguna razón s=0 (muy raro), igual lo dejamos
        results.push_back({id, s});
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.movieId < b.movieId;
              });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}
