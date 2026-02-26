#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <map>

std::string normalizar_texto(const std::string &texto) {
    // Mapa para reemplazar caracteres con tildes por sus equivalentes sin tilde
    static const std::map<std::string, char> tildes = {
        {"á", 'a'}, {"é", 'e'}, {"í", 'i'}, {"ó", 'o'}, {"ú", 'u'},
        {"Á", 'a'}, {"É", 'e'}, {"Í", 'i'}, {"Ó", 'o'}, {"Ú", 'u'},
        {"à", 'a'}, {"è", 'e'}, {"ì", 'i'}, {"ò", 'o'}, {"ù", 'u'},
        {"À", 'a'}, {"È", 'e'}, {"Ì", 'i'}, {"Ò", 'o'}, {"Ù", 'u'},
        {"ä", 'a'}, {"ë", 'e'}, {"ï", 'i'}, {"ö", 'o'}, {"ü", 'u'},
        {"Ä", 'a'}, {"Ë", 'e'}, {"Ï", 'i'}, {"Ö", 'o'}, {"Ü", 'u'},
        {"â", 'a'}, {"ê", 'e'}, {"î", 'i'}, {"ô", 'o'}, {"û", 'u'},
        {"Â", 'a'}, {"Ê", 'e'}, {"Î", 'i'}, {"Ô", 'o'}, {"Û", 'u'},
        {"ñ", 'n'}, {"Ñ", 'n'}
    };
    
    std::string resultado;
    size_t i = 0;
    
    while (i < texto.length()) {
        // Intentar buscar caracteres UTF-8 multibyte (tildes)
        bool encontrado = false;
        
        // Los caracteres con tilde en UTF-8 ocupan 2 bytes
        if (i + 1 < texto.length()) {
            std::string dos_bytes = texto.substr(i, 2);
            auto it = tildes.find(dos_bytes);
            if (it != tildes.end()) {
                resultado.push_back(it->second);
                i += 2;
                encontrado = true;
            }
        }
        
        if (!encontrado) {
            unsigned char c = texto[i];
            // Solo procesar caracteres ASCII válidos
            if (c < 128) {
                char lower = std::tolower(c);
                // Solo mantener letras, números y espacios (elimina puntuación)
                if (std::isalnum(lower) || std::isspace(lower)) {
                    resultado.push_back(lower);
                }
            }
            i++;
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