#include "SearchEngine.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

#include "CsvReader.h"


// --------------- containsAllTokens ----------------
bool SearchEngine::containsAllTokens(const std::string& text,
                                     const std::vector<std::string>& tokens) {
    for (const auto& tok : tokens) {
        if (tok.empty()) continue;
        if (text.find(tok) == std::string::npos) return false;
    }
    return true;
}

// --------------- computeIDF ----------------
double SearchEngine::computeIDF(const std::string& token) const {
    auto it = wordIndex.find(token);
    if (it == wordIndex.end()) return 0.0;

    double df = (double)it->second.size();
    double N  = (double)movies.size();
    return std::log((N + 1.0) / (df + 1.0));
}

// --------------- scoreUnifiedTokens ----------------
// Unificado para palabra y frase:
// - Título: TF (saturado) con peso alto
// - Sinopsis: TF-IDF (TF saturado) con peso
// - Bonos: AND por campo + frase exacta por campo (prioridad título>full)
// - Normalización por #tokens para no inflar frases largas
double SearchEngine::scoreUnifiedTokens(const Movie& m,
                                        const std::vector<std::string>& tokens,
                                        const std::string& query_norm) const
{
    const std::string& title = m.getTitle();
    const std::string& plot  = m.getPlot();
    const std::string& full  = m.getFullText();

    const double L = (double)std::max<size_t>(1, tokens.size()); // largo query

    // Pesos por campo (calibrables)
    constexpr double W_TITLE = 10.0; // título domina
    constexpr double W_PLOT  = 1.0;  // sinopsis aporta por TF-IDF

    // Bonos (NO enteros perfectos para evitar colapsar a 10.000 siempre)
    constexpr double AND_TITLE_BONUS = 9;
    constexpr double AND_PLOT_BONUS  = 4;
    constexpr double AND_FULL_BONUS  = 2;

    constexpr double PHRASE_TITLE_BONUS = 10;
    constexpr double PHRASE_FULL_BONUS  = 4;

    constexpr double LIKE_WEIGHT = 40;

    double score = 0.0;

    // --------- Score base por token ----------
    for (const auto& tok : tokens) {
        if (tok.empty()) continue;

        // Título: TF saturado (evita múltiplos de 10)
        int tf_title = countOccurrences(title, tok);
        score += W_TITLE * std::log1p((double)tf_title);

        // Sinopsis: TF-IDF con TF saturado
        double idf = computeIDF(tok);
        int tf_plot = countOccurrences(plot, tok);
        score += W_PLOT * std::log1p((double)tf_plot) * idf;
    }

    // --------- Bonos solo si es frase ----------
    if (tokens.size() > 1) {
        bool all_title = containsAllTokens(title, tokens);
        bool all_plot  = containsAllTokens(plot, tokens);
        bool all_full  = containsAllTokens(full, tokens);

        // AND por campo: escala por L para que no se "divida" a enteros al final
        if (all_title)      score += AND_TITLE_BONUS * L;
        else if (all_plot)  score += AND_PLOT_BONUS  * L;
        else if (all_full)  score += AND_FULL_BONUS  * L;

        // Frase exacta por campo: prioridad título > full
        if (title.find(query_norm) != std::string::npos)
            score += PHRASE_TITLE_BONUS * L;
        else if (full.find(query_norm) != std::string::npos)
            score += PHRASE_FULL_BONUS * L;
    }

    // Popularidad
    score += LIKE_WEIGHT * std::log1p((double)m.getLikes());

    // Normalización por largo query (evita inflar frases largas)
    score /= L;

    return score;
}



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


std::vector<SearchResult>
SearchEngine::searchSubstring(const std::string &q_norm,
                              size_t offset,
                              size_t limit) const {
    std::vector<SearchResult> empty;
    if (q_norm.empty()) return empty;

    // Tokens para scoring unificado (una palabra)
    std::vector<std::string> tokens = {q_norm};

    // ---- Caso corto: len < 3 (no hay trigramas) -> scan directo ----
    if (q_norm.size() < 3) {
        std::vector<SearchResult> results;
        results.reserve(256);

        for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
            const Movie &m = movies[i];
            if (m.getFullText().find(q_norm) != std::string::npos) {
                double s = scoreUnifiedTokens(m, tokens, q_norm);
                results.push_back({i, s});
            }
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

    // ---- 1) Candidatos por trigramas: intersección de postings ----
    std::vector<MovieId> cand;
    bool first = true;

    for (size_t i = 0; i + 3 <= q_norm.size(); ++i) {
        std::string gram = q_norm.substr(i, 3);
        const auto *p = trigramTrie.getPostings(gram);
        if (!p) return empty;

        if (first) {
            cand = *p; // postings ordenados+unique si llamaste finalize()
            first = false;
        } else {
            cand = intersectSorted(cand, *p);
        }

        if (cand.empty()) return empty;
    }

    // ---- 2) Verificación exacta + ranking unificado ----
    std::vector<SearchResult> results;
    results.reserve(std::min<size_t>(cand.size(), 4096));

    for (MovieId id: cand) {
        const Movie &m = movies[id];

        // verificación exacta (evita falsos positivos del filtro por n-gram)
        if (m.getFullText().find(q_norm) != std::string::npos) {
            double s = scoreUnifiedTokens(m, tokens, q_norm);
            results.push_back({id, s});
        }
    }

    // ---- 3) Orden y paginación ----
    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.movieId < b.movieId;
              });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}

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
static std::vector<std::string> tokenize_ws(const std::string &s) {
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

    for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
        // Indexamos palabras de título + sinopsis (ya normalizados en Movie)
        const std::string &text = movies[i].getFullText();
        auto tokens = tokenize_ws(text);

        for (const auto &w: tokens) {
            if (!w.empty()) wordIndex[w].push_back(i);
        }
    }

    // sort+unique por palabra para poder unir/intersectar rápido
    for (auto &kv: wordIndex) {
        sortUnique(kv.second);
    }
}

std::vector<SearchEngine::MovieId>
SearchEngine::unionSorted(const std::vector<MovieId> &a,
                          const std::vector<MovieId> &b) {
    std::vector<MovieId> out;
    out.reserve(a.size() + b.size());
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            out.push_back(a[i]);
            ++i;
            ++j;
        } else if (a[i] < b[j]) {
            out.push_back(a[i]);
            ++i;
        } else {
            out.push_back(b[j]);
            ++j;
        }
    }
    while (i < a.size()) out.push_back(a[i++]);
    while (j < b.size()) out.push_back(b[j++]);

    // out ya queda ordenado; puede tener repetidos si a/b no eran unique (pero lo son)
    // igual lo dejamos robusto:
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

int SearchEngine::countOccurrences(const std::string &text, const std::string &pattern) {
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
SearchEngine::searchPhrase(const std::string &query_norm,
                           size_t offset,
                           size_t limit) const
{
    std::vector<SearchResult> empty;
    if (query_norm.empty()) return empty;

    // 1) tokens
    auto tokens = tokenize_ws(query_norm);
    if (tokens.empty()) return empty;

    // 2) candidatos OR y AND usando wordIndex (rápido)
    std::vector<MovieId> cand_or;
    bool first_or = true;

    std::vector<MovieId> cand_and;
    bool and_initialized = false;

    for (const auto &t : tokens) {
        auto it = wordIndex.find(t);
        if (it == wordIndex.end()) continue;
        const auto &p = it->second;

        // OR
        if (first_or) { cand_or = p; first_or = false; }
        else          { cand_or = unionSorted(cand_or, p); }

        // AND
        if (!and_initialized) { cand_and = p; and_initialized = true; }
        else                  { cand_and = intersectSorted(cand_and, p); }
    }

    if (first_or) return empty; // ningún token existía en el índice

    auto in_and = [&](MovieId id) {
        if (!and_initialized) return false;
        return std::binary_search(cand_and.begin(), cand_and.end(), id);
    };

    // 3) ranking por campos (título > sinopsis), TF-IDF solo en plot
    constexpr double W_TITLE = 10.0;
    constexpr double W_PLOT  = 1.0;

    constexpr double AND_TITLE_BONUS = 30.0;
    constexpr double AND_PLOT_BONUS  = 15.0;
    constexpr double AND_FULL_BONUS  = 8.0;

    constexpr double PHRASE_TITLE_BONUS = 25.0;
    constexpr double PHRASE_FULL_BONUS  = 12.0;

    constexpr double LIKE_WEIGHT = 0.3;

    std::vector<SearchResult> results;
    results.reserve(std::min<size_t>(cand_or.size(), 10000));

    const double L = (double)std::max<size_t>(1, tokens.size());

    for (MovieId id : cand_or) {
        const Movie &m = movies[id];
        const std::string &title = m.getTitle();
        const std::string &plot  = m.getPlot();
        const std::string &full  = m.getFullText();

        double content = 0.0;

        // score base por token
        for (const auto &t : tokens) {
            if (t.empty()) continue;

            int tf_title = countOccurrences(title, t);
            int tf_plot  = countOccurrences(plot,  t);

            // título: TF saturado con peso alto (evita explotar)
            content += W_TITLE * std::log1p((double)tf_title);

            // sinopsis: TF-IDF plot-only (TF saturado)
            double idf = computeIDF(t);
            content += W_PLOT * std::log1p((double)tf_plot) * idf;
        }

        // normaliza solo el contenido (no los bonos)
        content /= L;

        double bonus = 0.0;

        // AND por campo con prioridad
        if (tokens.size() > 1) {
            bool all_title = containsAllTokens(title, tokens);
            bool all_plot  = containsAllTokens(plot, tokens);
            bool all_full  = containsAllTokens(full, tokens);

            if (all_title)      bonus += AND_TITLE_BONUS;
            else if (all_plot)  bonus += AND_PLOT_BONUS;
            else if (all_full)  bonus += AND_FULL_BONUS;

            // frase exacta: prioridad título > full
            if (title.find(query_norm) != std::string::npos)      bonus += PHRASE_TITLE_BONUS;
            else if (full.find(query_norm) != std::string::npos)  bonus += PHRASE_FULL_BONUS;
        }

        // likes suave
        double pop = LIKE_WEIGHT * std::log1p((double)m.getLikes());

        double s = content + bonus + pop;

        // Si además quieres usar tu AND del índice como señal extra (opcional):
        // if (in_and(id)) s += 1.0;

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

void SearchEngine::buildIndexes() {
    buildTrigramIndex();
    buildWordIndex();
    buildTagIndex();
}

void SearchEngine::load(const std::string &csvPath) {
    auto ms = CsvReader::cargar_csv(csvPath);
    setMovies(std::move(ms));
}
