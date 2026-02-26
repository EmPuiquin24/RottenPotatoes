#ifndef MOVIE_H 
#define MOVIE_H 

#include <string>
#include <vector>

class Movie {
private:
    std::string imbd_id;
    std::string title;
    std::string plot;
    std::vector<std::string> tags;
    std::string split;
    std::string synopsis_source;
public:
    Movie(const std::string& imbd_id, const std::string& title,
                const std::string& plot, const std::vector<std::string>& tags,
                const std::string& split, const std::string& synopsis_source)
        : imbd_id(imbd_id), title(title), plot(plot), tags(tags), split(split), synopsis_source(synopsis_source) {} 
};

#endif // MOVIE_H