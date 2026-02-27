#ifndef MOVIE_H
#define MOVIE_H

#include <string>
#include <vector>

class Movie {
private:
  std::string imdb_id, title, plot, split, synopsis_source;
  std::vector<std::string> tags;
  std::string full_text;
  int likes = 0;
  bool watch_later = false;

public:
  Movie(std::string imdb_id_, std::string title_, std::string plot_,
        std::vector<std::string> tags_, std::string split_, std::string source_)
      : imdb_id(std::move(imdb_id_)), title(std::move(title_)),
        plot(std::move(plot_)), tags(std::move(tags_)),
        split(std::move(split_)), synopsis_source(std::move(source_)) {
    full_text = title + " " + plot;
  }

  const std::string &getTitle() const { return title; }
  const std::string &getPlot() const { return plot; }
  const std::string &getFullText() const { return full_text; }
  const std::vector<std::string> &getTags() const { return tags; }

  int getLikes() const { return likes; }
  void like() { ++likes; }
  bool isWatchLater() const { return watch_later; }
  void setWatchLater(bool v) { watch_later = v; }


  const std::string& getImdbId() const { return imdb_id; }
};

#endif // MOVIE_H