#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "../movie/Movie.h"

std::string normalizar_texto(const std::string& texto);
std::vector<std::string> dividir(const std::string &cadena, char delimitador);

class CsvReader{
private:
    // Separa una fila del CSV respetando las comillas
    static std::vector<std::string> separarColumnas(const std::string& fila_logica) {
        std::vector<std::string> columnas;
        std::string celda;
        bool enComillas = false;

        for (char c : fila_logica) {
            if (c == '"') {
                enComillas = !enComillas;
            } else if (c == ',' && !enComillas) {
                columnas.push_back(celda);
                celda.clear();
            } else {
                celda += c;
            }
        }
        columnas.push_back(celda);
        return columnas;
    }
public:
    static std::vector<Movie> cargar_csv(const std::string &nombre_archivo) {
        std::ifstream archivo(nombre_archivo);
        std::vector<Movie> Movies;
        
        if (!archivo.is_open()) {
            std::cerr << "Error: No se pudo abrir el archivo." << std::endl;
            std::terminate();
        }

        std::string linea_fisica;
        std::string fila_logica = "";
        int contador_comillas = 0;

        // Saltar cabecera
        getline(archivo, linea_fisica);

        // Leer archivo y arreglar saltos de linea dentro de comillas
        while (getline(archivo, linea_fisica)) {
            fila_logica += linea_fisica;
            
            contador_comillas += std::count(linea_fisica.begin(), linea_fisica.end(), '"');

            // Si el numero de comillas es par, la fila esta completa
            if (contador_comillas % 2 == 0) {
                std::vector<std::string> columnas = separarColumnas(fila_logica);

                if (columnas.size() >= 6) {
                    std::string imdb_id = columnas[0];
                    std::string titulo = columnas[1];
                    std::string sinopsis = columnas[2];
                    std::string etiquetas = columnas[3];
                    std::string split_val = columnas[4];
                    std::string source_val = columnas[5];

                    std::vector<std::string> etiquetas_procesadas = dividir(etiquetas, ',');
                    
                    Movie Movie(imdb_id, normalizar_texto(titulo),
                                      normalizar_texto(sinopsis), etiquetas_procesadas,
                                      split_val, source_val);
                                      
                    Movies.push_back(Movie);
                }

                fila_logica = "";
                contador_comillas = 0;
            }
        }

        archivo.close();
        return Movies;
    }
};