#include "csvreader/CsvReader.h"


int main() {

    std::string file = "../resources/mpst_full_data.csv";
    std::vector<Movie> movies = CsvReader::cargar_csv(file);
    
    std::cout << "Número de películas cargadas: " << movies.size() << std::endl;
    
    return 0; 
}

