#include <string>
#include <vector>
#include <sstream>

std::string normalizar_texto(const std::string &texto) {
    std::string resultado;
    for (char c : texto) {
        c = tolower(c);
        if (isalnum(c) || isspace(c)) {
            resultado.push_back(c);
        }
    }
    return resultado;
}

// 2. Función para dividir las etiquetas (tags) separadas por comas
std::vector<std::string> dividir(const std::string &cadena, char delimitador) {
    std::vector<std::string> tokens;
    std::stringstream ss(cadena);
    std::string token;
    while (getline(ss, token, delimitador)) {
        tokens.push_back(normalizar_texto(token));
    }
    return tokens;
}