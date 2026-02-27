#ifndef NGRAM_TRIE_H
#define NGRAM_TRIE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

class NGramTrie {
public:
    using MovieId = int;

private:
    struct Node {
        std::unordered_map<char, int> next;
        std::vector<MovieId> postings;
    };

    std::vector<Node> nodes;

public:
    NGramTrie() { nodes.emplace_back(); }

    void insert(const std::string& gram, MovieId movieId) {
        int u = 0;
        for (char ch : gram) {
            if (nodes[u].next.find(ch) == nodes[u].next.end()) {
                nodes[u].next[ch] = (int)nodes.size();
                nodes.emplace_back();
            }
            u = nodes[u].next[ch];
        }
        nodes[u].postings.push_back(movieId);
    }

    const std::vector<MovieId>* getPostings(const std::string& gram) const {
        int u = 0;
        for (char ch : gram) {
            auto it = nodes[u].next.find(ch);
            if (it == nodes[u].next.end()) return nullptr;
            u = it->second;
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
};

#endif