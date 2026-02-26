#ifndef NGRAM_TRIE_H
#define NGRAM_TRIE_H

#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

class NGramTrie {
public:
    using MovieId = uint32_t;

private:
    struct Node {
        std::array<int, 128> next;      // ASCII básico
        std::vector<MovieId> postings;  // ids de películas donde aparece este ngram
        Node() { next.fill(-1); }
    };

    std::vector<Node> nodes;

public:
    NGramTrie() { nodes.emplace_back(); } // root

    void insert(const std::string& gram, MovieId movieId) {
        int u = 0;
        for (unsigned char ch : gram) {
            if (ch >= 128) return; 
            if (nodes[u].next[ch] == -1) {
                nodes[u].next[ch] = (int)nodes.size();
                nodes.emplace_back();
            }
            u = nodes[u].next[ch];
        }
        nodes[u].postings.push_back(movieId);
    }

    const std::vector<MovieId>* getPostings(const std::string& gram) const {
        int u = 0;
        for (unsigned char ch : gram) {
            if (ch >= 128) return nullptr;
            int v = nodes[u].next[ch];
            if (v == -1) return nullptr;
            u = v;
        }
        return &nodes[u].postings;
    }

    void finalize() {
        for (auto &nd : nodes) {
            auto &p = nd.postings;
            if (p.empty()) continue;
            std::sort(p.begin(), p.end());
            p.erase(std::unique(p.begin(), p.end()), p.end());
        }
    }

    size_t nodeCount() const { return nodes.size(); }
};

#endif