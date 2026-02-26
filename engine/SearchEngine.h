#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "../movie/Movie.h"
#include "../index/NGramTrie.h"


struct SearchResult {
    uint32_t movieId;
    double score;
};

class SearchEngine {
public:
    using MovieId = uint32_t;

private:
    std::vector<Movie> movies;
    NGramTrie trigramTrie;

    std::unordered_map<std::string, std::vector<MovieId> > tagIndex;

    static std::vector<MovieId> intersectSorted(const std::vector<MovieId> &a,
                                                const std::vector<MovieId> &b);

    static double scoreMovie(const Movie &m, const std::string &q);

    static void sortUnique(std::vector<MovieId> &v) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
    }

public:
    void setMovies(std::vector<Movie> &&loaded) { movies = std::move(loaded); }

    const std::vector<Movie> &getMovies() const { return movies; }
    std::vector<Movie> &getMovies() { return movies; }

    void buildTrigramIndex(); // (title+plot)
    void buildTagIndex();

    // Devuelve IDs rankeados, con paginación por offset/limit
    std::vector<SearchResult> searchSubstring(const std::string &q_norm,
                                              size_t offset = 0,
                                              size_t limit = 5) const;

    std::vector<SearchResult> searchByTag(const std::string &tag_norm,
                                          size_t offset = 0,
                                          size_t limit = 5) const;
};

#endif
