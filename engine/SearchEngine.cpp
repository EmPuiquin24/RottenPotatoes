#include "SearchEngine.h"
#include <algorithm>
#include <cctype>
#include <sstream>

#include "../csvreader/CsvReader.h"

SearchEngine* SearchEngine::instance = nullptr;
std::mutex SearchEngine::mutex_instance;

bool SearchEngine::containsAllTokens(const std::string& text, const std::vector<std::string>& tokens) {
    for (const auto& tok : tokens) {
        if (tok.empty()) continue;
        if (text.find(tok) == std::string::npos) return false;
    }
    return true;
}

int SearchEngine::countOccurrences(const std::string &text, const std::string &pattern) {
    if (pattern.empty()) return 0;
    int cnt = 0;
    size_t pos = 0;
    while (true) {
        pos = text.find(pattern, pos);
        if (pos == std::string::npos) break;
        ++cnt;
        pos += pattern.size();
    }
    return cnt;
}

double SearchEngine::scoreUnifiedTokens(const Movie& m, const std::vector<std::string>& tokens, const std::string& query_norm) const {
    const std::string& title = m.getTitle();
    const std::string& plot  = m.getPlot();

    int score = 0;

    for (const auto& tok : tokens) {
        if (tok.empty()) continue;
        if (title.find(tok) != std::string::npos) {
            score += 10;
        }
        score += countOccurrences(plot, tok);
    }

    return (double)score;
}

void SearchEngine::buildTrigramIndex() {
    for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
        const std::string &text = movies[i].getFullText();
        if (text.size() < 3) continue;

        for (size_t j = 0; j + 3 <= text.size(); ++j) {
            trigramTrie.insert(text.substr(j, 3), i);
        }
    }
    trigramTrie.finalize();
}

std::vector<SearchEngine::MovieId> SearchEngine::intersectSorted(const std::vector<MovieId> &a, const std::vector<MovieId> &b) {
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

std::vector<SearchResult> SearchEngine::searchSubstring(const std::string &q_norm, size_t offset, size_t limit) const {
    std::vector<SearchResult> empty;
    if (q_norm.empty()) return empty;

    std::vector<std::string> tokens = {q_norm};

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

        std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
            if (a.score != b.score) return a.score > b.score;
            return a.movieId < b.movieId;
        });

        if (offset >= results.size()) return empty;
        size_t end = std::min(results.size(), offset + limit);
        return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
    }

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

    std::vector<SearchResult> results;
    results.reserve(std::min<size_t>(cand.size(), 4096));

    for (MovieId id: cand) {
        const Movie &m = movies[id];

        if (m.getFullText().find(q_norm) != std::string::npos) {
            double s = scoreUnifiedTokens(m, tokens, q_norm);
            results.push_back({id, s});
        }
    }

    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
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

            std::string tag_norm = tag;
            std::transform(tag_norm.begin(), tag_norm.end(), tag_norm.begin(),
                           [](unsigned char c) { return std::tolower(c); });

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

std::vector<SearchResult> SearchEngine::searchByTag(const std::string &tag_norm, size_t offset, size_t limit) const {
    std::vector<SearchResult> empty;
    if (tag_norm.empty()) return empty;

    auto it = tagIndex.find(tag_norm);
    if (it == tagIndex.end()) return empty;

    const auto &ids = it->second;

    std::vector<SearchResult> results;
    results.reserve(ids.size());

    for (MovieId id: ids) {
        const Movie &m = movies[id];
        int s = m.getLikes();
        results.push_back({id, (double)s});
    }

    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.movieId < b.movieId;
    });

    if (offset >= results.size()) return empty;
    size_t end = std::min(results.size(), offset + limit);
    return std::vector<SearchResult>(results.begin() + offset, results.begin() + end);
}

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
    wordIndex.reserve(movies.size());
    
    for (MovieId i = 0; i < (MovieId) movies.size(); ++i) {
        const std::string &text = movies[i].getFullText();
        auto tokens = tokenize_ws(text);
        for (const auto &w: tokens) {
            if (!w.empty()) wordIndex[w].push_back(i);
        }
    }

    for (auto &kv: wordIndex) {
        sortUnique(kv.second);
    }
}

std::vector<SearchEngine::MovieId> SearchEngine::unionSorted(const std::vector<MovieId> &a, const std::vector<MovieId> &b) {
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

    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::vector<SearchResult> SearchEngine::searchPhrase(const std::string &query_norm, size_t offset, size_t limit) const {
    std::vector<SearchResult> empty;
    if (query_norm.empty()) return empty;

    auto tokens = tokenize_ws(query_norm);
    if (tokens.empty()) return empty;

    std::vector<MovieId> cand_or;
    bool first_or = true;

    for (const auto &t : tokens) {
        auto it = wordIndex.find(t);
        if (it == wordIndex.end()) continue;
        const auto &p = it->second;

        if (first_or) { cand_or = p; first_or = false; }
        else          { cand_or = unionSorted(cand_or, p); }
    }

    if (first_or) return empty;

    std::vector<SearchResult> results;
    results.reserve(cand_or.size());

    for (MovieId id : cand_or) {
        const Movie &m = movies[id];
        const std::string &title = m.getTitle();
        const std::string &plot  = m.getPlot();

        int score = 0;

        for (const auto &t : tokens) {
            if (t.empty()) continue;
            if (title.find(t) != std::string::npos) {
                score += 10;
            }
            score += countOccurrences(plot, t);
        }

        if (title.find(query_norm) != std::string::npos) {
            score += 20;
        }

        results.push_back({id, (double)score});
    }

    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
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
