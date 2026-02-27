#ifndef QUERY_BUILDER_H
#define QUERY_BUILDER_H

#include <string>
#include <vector>
#include <sstream>

// Patrón Builder para construir queries complejas de búsqueda

class SearchQuery {
public:
    std::string text;
    std::vector<std::string> tags;
    bool isPhrase;
    size_t offset;
    size_t limit;
    int minYear;
    int maxYear;
    bool filterByLikes;
    int minLikes;
    
    SearchQuery() 
        : isPhrase(false), offset(0), limit(10), 
          minYear(0), maxYear(9999), filterByLikes(false), minLikes(0) {}
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "SearchQuery{";
        oss << "text='" << text << "', ";
        oss << "tags=[";
        for (size_t i = 0; i < tags.size(); ++i) {
            oss << tags[i];
            if (i + 1 < tags.size()) oss << ", ";
        }
        oss << "], ";
        oss << "isPhrase=" << isPhrase << ", ";
        oss << "offset=" << offset << ", ";
        oss << "limit=" << limit;
        if (filterByLikes) {
            oss << ", minLikes=" << minLikes;
        }
        oss << "}";
        return oss.str();
    }
};

class QueryBuilder {
private:
    SearchQuery query;
    
public:
    QueryBuilder() = default;
    
    QueryBuilder& setText(const std::string& text) {
        query.text = text;
        return *this;
    }
    
    QueryBuilder& addTag(const std::string& tag) {
        query.tags.push_back(tag);
        return *this;
    }
    
    QueryBuilder& setTags(const std::vector<std::string>& tags) {
        query.tags = tags;
        return *this;
    }
    
    QueryBuilder& asPhrase() {
        query.isPhrase = true;
        return *this;
    }
    
    QueryBuilder& asSubstring() {
        query.isPhrase = false;
        return *this;
    }
    
    QueryBuilder& setOffset(size_t offset) {
        query.offset = offset;
        return *this;
    }
    
    QueryBuilder& setLimit(size_t limit) {
        query.limit = limit;
        return *this;
    }
    
    QueryBuilder& setPage(size_t pageNumber, size_t pageSize = 10) {
        query.offset = pageNumber * pageSize;
        query.limit = pageSize;
        return *this;
    }
    
    QueryBuilder& setYearRange(int minYear, int maxYear) {
        query.minYear = minYear;
        query.maxYear = maxYear;
        return *this;
    }
    
    QueryBuilder& filterByMinLikes(int minLikes) {
        query.filterByLikes = true;
        query.minLikes = minLikes;
        return *this;
    }
    
    SearchQuery build() {
        return query;
    }
    
    // Método para resetear el builder
    void reset() {
        query = SearchQuery();
    }
};

#endif // QUERY_BUILDER_H
