#include "SearchEngine.h"
#include <algorithm>
#include <cmath>

void SearchEngine::buildTrigramIndex() {
    // Construye trigramas (n=3) sobre full_text
    for (MovieId i = 0; i < (MovieId)movies.size(); ++i) {
        const std::string& text = movies[i].getFullText();
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
SearchEngine::intersectSorted(const std::vector<MovieId>& a,
                              const std::vector<MovieId>& b) {
    std::vector<MovieId> out;
    out.reserve(std::min(a.size(), b.size()));
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) { out.push_back(a[i]); ++i; ++j; }
        else if (a[i] < b[j]) ++i;
        else ++j;
    }
    return out;
}

double SearchEngine::scoreMovie(const Movie& m, const std::string& q) {
    // Ranking simple y defendible
    // +10 si aparece en title, +3 si aparece en plot, +log(1+likes)
    double s = 0.0;
    if (m.getTitle().find(q) != std::string::npos) s += 10.0;
    if (m.getPlot().find(q) != std::string::npos)  s += 3.0;
    s += std::log(1.0 + (double)m.getLikes());
    return s;
}

std::vector<SearchResult>
SearchEngine::searchSubstring(const std::string& q_norm,
                              size_t offset,
                              size_t limit) const {
    std::vector<SearchResult> empty;
    if (q_norm.empty()) return empty;

    // Queries cortas: por ahora fallback a scan directo (para que funcione ya)
    // Luego lo mejoramos con word index.
    if (q_norm.size() < 3) {
        std::vector<SearchResult> all;
        all.reserve(256);
        for (MovieId i = 0; i < (MovieId)movies.size(); ++i) {
            if (movies[i].getFullText().find(q_norm) != std::string::npos) {
                all.push_back({i, scoreMovie(movies[i], q_norm)});
            }
        }
        std::sort(all.begin(), all.end(), [](auto& x, auto& y){
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
        const auto* p = trigramTrie.getPostings(gram);
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

    for (MovieId id : cand) {
        const Movie& m = movies[id];
        if (m.getFullText().find(q_norm) != std::string::npos) {
            results.push_back({id, scoreMovie(m, q_norm)});
        }
    }

    // 3) ordenar por score desc y paginar
    std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b){
        if (a.score != b.score) return a.score > b.score;
        return a.movieId < b.movieId;
    });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}