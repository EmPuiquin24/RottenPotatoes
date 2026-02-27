#ifndef INDEX_FACTORY_H
#define INDEX_FACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include "../index/NGramTrie.h"

// Patrón Factory Method para crear diferentes tipos de índices

// Interfaz base para índices
template<typename IdType>
class Index {
public:
    virtual ~Index() = default;
    virtual void build(const std::vector<std::string>& documents) = 0;
    virtual std::vector<IdType> search(const std::string& query) const = 0;
    virtual std::string getType() const = 0;
};

// Implementación de índice de trigramas
template<typename IdType>
class TrigramIndex : public Index<IdType> {
private:
    NGramTrie trie;
    
public:
    void build(const std::vector<std::string>& documents) override {
        for (IdType i = 0; i < documents.size(); ++i) {
            const std::string& text = documents[i];
            if (text.size() < 3) continue;
            
            for (size_t j = 0; j + 3 <= text.size(); ++j) {
                trie.insert(text.substr(j, 3), i);
            }
        }
        trie.finalize();
    }
    
    std::vector<IdType> search(const std::string& query) const override {
        if (query.size() < 3) return {};
        
        const auto* postings = trie.getPostings(query.substr(0, 3));
        return postings ? *postings : std::vector<IdType>();
    }
    
    std::string getType() const override {
        return "TrigramIndex";
    }
};

// Implementación de índice invertido
template<typename IdType>
class InvertedIndex : public Index<IdType> {
private:
    std::unordered_map<std::string, std::vector<IdType>> index;
    
public:
    void build(const std::vector<std::string>& documents) override {
        for (IdType i = 0; i < documents.size(); ++i) {
            const std::string& text = documents[i];
            std::istringstream iss(text);
            std::string word;
            
            while (iss >> word) {
                if (!word.empty()) {
                    index[word].push_back(i);
                }
            }
        }
        
        // Sort y unique para cada palabra
        for (auto& kv : index) {
            std::sort(kv.second.begin(), kv.second.end());
            kv.second.erase(std::unique(kv.second.begin(), kv.second.end()), 
                           kv.second.end());
        }
    }
    
    std::vector<IdType> search(const std::string& query) const override {
        auto it = index.find(query);
        return it != index.end() ? it->second : std::vector<IdType>();
    }
    
    std::string getType() const override {
        return "InvertedIndex";
    }
};

// Factory para crear índices
template<typename IdType>
class IndexFactory {
public:
    enum class IndexType {
        TRIGRAM,
        INVERTED
    };
    
    static std::unique_ptr<Index<IdType>> createIndex(IndexType type) {
        switch (type) {
            case IndexType::TRIGRAM:
                return std::make_unique<TrigramIndex<IdType>>();
            case IndexType::INVERTED:
                return std::make_unique<InvertedIndex<IdType>>();
            default:
                return nullptr;
        }
    }
};

#endif // INDEX_FACTORY_H
