#ifndef OBSERVER_H
#define OBSERVER_H

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

// Patron Observer: cuando hay un cambio (like, watchlist), se notifica a los observers

class Observer {
public:
    virtual ~Observer() = default;
    virtual void onMovieLiked(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieUnliked(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieAddedToWatchlist(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) = 0;
};

class Subject {
private:
    std::vector<Observer*> observers;
    
public:
    void attach(Observer* observer) {
        observers.push_back(observer);
    }
    
    void detach(Observer* observer) {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end()
        );
    }
    
    void notifyMovieLiked(int movieId, const std::string& movieTitle) {
        for (auto* observer : observers) {
            observer->onMovieLiked(movieId, movieTitle);
        }
    }
    
    void notifyMovieUnliked(int movieId, const std::string& movieTitle) {
        for (auto* observer : observers) {
            observer->onMovieUnliked(movieId, movieTitle);
        }
    }
    
    void notifyMovieAddedToWatchlist(int movieId, const std::string& movieTitle) {
        for (auto* observer : observers) {
            observer->onMovieAddedToWatchlist(movieId, movieTitle);
        }
    }
    
    void notifyMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) {
        for (auto* observer : observers) {
            observer->onMovieRemovedFromWatchlist(movieId, movieTitle);
        }
    }
};

// Observer que imprime logs en consola
class ConsoleLogger : public Observer {
public:
    void onMovieLiked(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] <3 Te gusto: '" << movieTitle << "'\n";
    }
    
    void onMovieUnliked(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] </3 Ya no te gusta: '" << movieTitle << "'\n";
    }
    
    void onMovieAddedToWatchlist(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] [WL] Agregado a ver mas tarde: '" << movieTitle << "'\n";
    }
    
    void onMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] [OK] Removido de ver mas tarde: '" << movieTitle << "'\n";
    }
};

#endif // OBSERVER_H
