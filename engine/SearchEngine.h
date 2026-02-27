#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "../movie/Movie.h"
#include "../index/NGramTrie.h"


struct SearchResult {
    uint32_t movieId;
    double score;
};

// Patrón Singleton
class SearchEngine {
public:
    using MovieId = uint32_t;

    // Método estático para obtener la instancia única
    static SearchEngine* getInstance() {
        if (instance == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_instance);
            if (instance == nullptr) {
                instance = new SearchEngine();
            }
        }
        return instance;
    }

    // Eliminar constructor de copia y operador de asignación
    SearchEngine(const SearchEngine&) = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;

private:
    static SearchEngine* instance;
    static std::mutex mutex_instance;

    std::vector<Movie> movies;
    NGramTrie trigramTrie;
    std::unordered_map<std::string, std::vector<MovieId> > tagIndex;
    std::unordered_map<std::string, std::vector<MovieId> > wordIndex;

    // Constructor privado para Singleton
    SearchEngine() = default;

    static std::vector<MovieId> intersectSorted(const std::vector<MovieId> &a,
                                                const std::vector<MovieId> &b);


    static void sortUnique(std::vector<MovieId> &v) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
    }


    static std::vector<MovieId> unionSorted(const std::vector<MovieId> &a,
                                            const std::vector<MovieId> &b);

    static int countOccurrences(const std::string &text, const std::string &pattern);

public:
    void setMovies(std::vector<Movie> &&loaded) { movies = std::move(loaded); }

    const std::vector<Movie> &getMovies() const { return movies; }
    std::vector<Movie> &getMovies() { return movies; }

    void buildTrigramIndex(); // (title+plot)
    void buildTrigramIndexParallel(); // Version paralela
    void buildTagIndex();
    void buildTagIndexParallel(); // Version paralela
    void buildWordIndex();
    void buildWordIndexParallel(); // Version paralela

    // Devuelve IDs rankeados, con paginación por offset/limit
    std::vector<SearchResult> searchSubstring(const std::string &q_norm,
                                              size_t offset = 0,
                                              size_t limit = 5) const;

    std::vector<SearchResult> searchByTag(const std::string &tag_norm,
                                          size_t offset = 0,
                                          size_t limit = 5) const;


    std::vector<SearchResult> searchPhrase(const std::string &query_norm,
                                           size_t offset = 0,
                                           size_t limit = 5) const;


    double computeIDF(const std::string &token) const;


    static bool containsAllTokens(const std::string &text,
                                  const std::vector<std::string> &tokens);

    double scoreUnifiedTokens(const Movie &m,
                              const std::vector<std::string> &tokens,
                              const std::string &query_norm) const;

    const Movie &getMovieById(int id) const {
        return movies.at((size_t) id);
    }

    size_t movieCount() const {
        return movies.size();
    }

    void buildIndexes();
    void buildIndexesParallel(); // Version paralela
    void load(const std::string& csvPath);
};

#endif
