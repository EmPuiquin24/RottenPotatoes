#ifndef MOVIE_OBSERVER_H
#define MOVIE_OBSERVER_H

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

// Patron Observer para notificar cambios en peliculas (likes, watchlist, etc.)

// Interfaz Observer
class MovieObserver {
public:
    virtual ~MovieObserver() = default;
    virtual void onMovieLiked(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieUnliked(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieAddedToWatchlist(int movieId, const std::string& movieTitle) = 0;
    virtual void onMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) = 0;
};

// Sujeto Observable
class MovieSubject {
private:
    std::vector<MovieObserver*> observers;
    
public:
    void attach(MovieObserver* observer) {
        observers.push_back(observer);
    }
    
    void detach(MovieObserver* observer) {
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

// Implementacion concreta: Logger que imprime las notificaciones
class ConsoleLogger : public MovieObserver {
public:
    void onMovieLiked(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] <3 Te gusto: '" << movieTitle << "' (ID: " << movieId << ")\n";
    }
    
    void onMovieUnliked(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] </3 Ya no te gusta: '" << movieTitle << "' (ID: " << movieId << ")\n";
    }
    
    void onMovieAddedToWatchlist(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] [WL] Agregado a ver mas tarde: '" << movieTitle << "' (ID: " << movieId << ")\n";
    }
    
    void onMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) override {
        std::cout << "[LOG] [OK] Removido de ver mas tarde: '" << movieTitle << "' (ID: " << movieId << ")\n";
    }
};

// Implementacion concreta: Estadisticas
class StatisticsTracker : public MovieObserver {
private:
    int totalLikes = 0;
    int totalUnlikes = 0;
    int totalWatchlistAdds = 0;
    int totalWatchlistRemoves = 0;
    
public:
    void onMovieLiked(int movieId, const std::string& movieTitle) override { totalLikes++; }
    void onMovieUnliked(int movieId, const std::string& movieTitle) override { totalUnlikes++; }
    void onMovieAddedToWatchlist(int movieId, const std::string& movieTitle) override { totalWatchlistAdds++; }
    void onMovieRemovedFromWatchlist(int movieId, const std::string& movieTitle) override { totalWatchlistRemoves++; }

    void printStatistics() const {
        std::cout << "\n=== Estadisticas de Usuario ===\n";
        std::cout << "Total de likes:              " << totalLikes << "\n";
        std::cout << "Total de unlikes:            " << totalUnlikes << "\n";
        std::cout << "Agregados a watchlist:       " << totalWatchlistAdds << "\n";
        std::cout << "Removidos de watchlist:      " << totalWatchlistRemoves << "\n";
        std::cout << "==============================\n\n";
    }
};

#endif // MOVIE_OBSERVER_H
