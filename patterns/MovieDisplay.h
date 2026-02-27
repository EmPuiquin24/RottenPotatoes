#ifndef MOVIE_DISPLAY_H
#define MOVIE_DISPLAY_H

#include <iostream>
#include <string>
#include "../movie/Movie.h"

// Patron Decorator: para mostrar detalles de peliculas con diferentes niveles de info
class MovieDisplay {
public:
    virtual void display(const Movie& m) = 0;
    virtual ~MovieDisplay() {}
};

// Display basico
class BasicMovieDisplay : public MovieDisplay {
public:
    void display(const Movie& m) override {
        std::cout << "Titulo: " << m.getTitle() << "\n";
        std::cout << "Sinopsis: " << m.getPlot() << "\n";
        
        const auto& tags = m.getTags();
        if (!tags.empty()) {
            std::cout << "Generos: ";
            for (size_t i = 0; i < tags.size(); ++i) {
                std::cout << tags[i];
                if (i + 1 < tags.size()) std::cout << ", ";
            }
            std::cout << "\n";
        }
    }
};

// Decorator base
class MovieDisplayDecorator : public MovieDisplay {
protected:
    MovieDisplay* wrapped;
    
public:
    MovieDisplayDecorator(MovieDisplay* m) : wrapped(m) {}
    virtual ~MovieDisplayDecorator() { delete wrapped; }
    
    void display(const Movie& m) override {
        wrapped->display(m);
    }
};

// Display extendido con IMDB ID
class ExtendedMovieDisplay : public MovieDisplayDecorator {
public:
    ExtendedMovieDisplay(MovieDisplay* m) : MovieDisplayDecorator(m) {}
    
    void display(const Movie& m) override {
        MovieDisplayDecorator::display(m);
        std::cout << "IMDB ID: " << m.getImdbId() << "\n";
    }
};

#endif
