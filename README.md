# Programación III: Proyecto Final (2026-0)
## Integrantes: 
- Hector Emilio Huaman Puiquin
- Gerald Fernando Marcelo Borjas Bernaola 
- Sebastian Vir Briceño Gora
- Paul Ricardo Quispe Maguiña

## Tabla de Contenidos
- [Introducción](#introducción)
    - [Objetivo](#objetivo)
    - [Motivación](#motivación)
    - [Alcance](#alcance)
- [Requisitos](#requisitos)
- [Instalación](#instalación)
- [Manual de Uso](#manual-de-uso)
- [Documentación](#documentación)
    - [Diagrama General](#diagrama-general)
    - [Limpieza de Datos](#limpieza-de-datos)
    - [Características](#características)
    - [Arquitectura](#arquitectura)
    - [Patrones de Diseño](#patrones-de-diseño)
    - [Rendimiento](#rendimiento)
- [Proceso de Desarrollo](#proceso-de-desarrollo)
- [Contribución](#contribución)

## Introducción
### Objetivo
RottenPotatoes es una plataforma de búsqueda de películas que permite buscar por palabras, frases o géneros. El sistema usa un Trie de n-gramas para búsquedas rápidas y permite a los usuarios guardar sus películas favoritas.

### Motivación
Queríamos implementar estructuras de datos avanzadas (Tries) y patrones de diseño mientras trabajábamos en equipo. El proyecto nos permitió aprender sobre búsquedas eficientes, programación paralela básica y arquitectura de software.

### Alcance
- Búsqueda por palabra o frase
- Búsqueda por géneros/tags
- Sistema de likes y watchlist
- Persistencia de datos en archivos
- Ranking de resultados por relevancia

## Requisitos
### Requisitos de Software
- **Compilador C++20** o superior
- **CMake 3.29+**
- **Sistema operativo**: Linux, macOS o Windows

### Requisitos de Hardware
No hay requisitos especiales. El programa funciona en cualquier computadora moderna.

## Instalación
Clonar el repositorio:
```bash
git clone git@github.com:EmPuiquin24/RottenPotatoes.git
cd RottenPotatoes 
```

Compilar con CMake:
```bash
mkdir build && cd build
cmake ..
make
```

Ejecutar:
```bash
./RottenPotatoes
```

## Manual de Uso
Al ejecutar el programa verás este menú:

```bash
=== Menu Principal ===
1) Buscar pelicula por palabra
2) Buscar pelicula por frase
3) Buscar por etiqueta (tag)
4) Mi lista de Ver mas tarde
5) Ver peliculas likeadas
6) Salir

Escribe tu opcion: 
```

### Buscar películas
Selecciona opción 1 o 2, luego escribe tu búsqueda:
```bash
Query: batman
```

Verás los resultados paginados:
```bash
Resultados (SUBSTRING) query="batman"  [offset=0]
------------------------------------------------------------
      0  batman begins  [score=30.000]
      5  batman returns  [score=25.000]
     12  the dark knight  [score=20.000]

Opciones:
  1) Ver detalles (por ID)
  2) Siguiente pagina
  3) Pagina anterior
  4) Salir a menu
```

### Ver detalles
Selecciona opción 1 e ingresa el ID de la película:
```bash
Ingresa movieId: 0

------------------------------------------------------------
ID: 0
Titulo: batman begins
Liked: NO
Ver mas tarde: NO
Tags: action, superhero

------------------------------------------------------------
SINOPSIS:
a young bruce wayne travels to the far east where he is trained...
------------------------------------------------------------

Acciones:
  1) Toggle Like (reordena resultados)
  2) Toggle Ver mas tarde
  3) Volver a resultados
```

## Documentación
### Diagrama General
```
main.cpp
    │
    ├── SearchEngine (Singleton)
    │   ├── NGramTrie (búsqueda rápida)
    │   ├── wordIndex (índice invertido)
    │   └── tagIndex (búsqueda por género)
    │
    ├── MovieDisplay (Decorator)
    │   └── Muestra detalles de películas
    │
    └── Observer
        └── Notifica acciones (likes, watchlist)
```

### Limpieza de Datos
El dataset original tenía caracteres especiales y formato inconsistente. Usamos normalización de texto en C++:

```cpp
// Quita tildes, convierte a minúsculas, elimina símbolos
std::string normalizar_texto(const std::string &texto);
```

### Características
#### Búsqueda con Trie
Implementamos un Trie de trigramas (grupos de 3 letras) que permite:
- Búsquedas de substrings en O(k) donde k = longitud de búsqueda
- Filtrado rápido de candidatos antes de verificación exacta

Ejemplo: para "batman" se generan trigramas: "bat", "atm", "tma", "man"

#### Interfaz CLI
Menú simple en terminal con navegación intuitiva por páginas de resultados.

#### Sistema de Ranking
Las películas se ordenan por score:
- Palabra en título: +10 puntos
- Palabra en sinopsis: +1 punto
- Frase exacta en título: +20 puntos
- Películas con like del usuario: +12 puntos adicionales

### Arquitectura
#### Estructura de Datos Principal
**NGramTrie**: Árbol que indexa trigramas para búsqueda rápida
```cpp
class NGramTrie {
    struct Node {
        unordered_map<char, int> next;
        vector<MovieId> postings; // IDs de películas
    };
};
```

**Índices Auxiliares**:
- `wordIndex`: mapea palabras → películas
- `tagIndex`: mapea géneros → películas

#### Clases Principales
- **Movie**: Representa una película (título, sinopsis, tags)
- **SearchEngine**: Motor de búsqueda (Singleton)
- **CsvReader**: Lee y parsea el CSV
- **Observer**: Notifica acciones del usuario

#### Uso de Threads
Usamos un thread para cargar el CSV en paralelo:
```cpp
std::thread cargaThread([&engine, &path]() {
    engine->load(path);
    engine->buildIndexes();
});
cargaThread.join();
```

### Patrones de Diseño
#### 1. Singleton
Garantiza una sola instancia del motor de búsqueda:
```cpp
static SearchEngine* getInstance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_instance);
        if (instance == nullptr) {
            instance = new SearchEngine();
        }
    }
    return instance;
}
```
**Por qué**: Evitamos duplicar los índices en memoria (que son pesados) y centralizamos el acceso.

#### 2. Observer
Notifica cuando el usuario hace like o agrega a watchlist:
```cpp
class ConsoleLogger : public Observer {
    void onMovieLiked(int id, const string& title) override {
        cout << "[LOG] Te gustó: " << title << "\n";
    }
};
```
**Por qué**: El UserState no necesita saber quién escucha. Solo notifica y los observers hacen su trabajo (logging, stats, etc).

#### 3. Decorator  
Agrega funcionalidad a la visualización de películas sin modificar la clase base:
```cpp
// Display básico
MovieDisplay* display = new BasicMovieDisplay();

// Decorator que agrega IMDB ID
display = new ExtendedMovieDisplay(display);
display->display(movie);
```
**Por qué**: Podemos agregar o quitar información mostrada de forma flexible sin tocar el código base.

### Rendimiento
#### Complejidad Temporal

| Operación | Complejidad |
|-----------|-------------|
| Carga CSV | O(N × L) donde N = películas, L = longitud |
| Build Trigram Index | O(N × L) |
| Búsqueda substring | O(k + C log C) donde k = trigramas, C = candidatos |
| Búsqueda por tag | O(1 + R log R) donde R = resultados |

#### Tabla de Comparación (dataset 100K películas)

| Operación | Tiempo |
|-----------|--------|
| Carga + construcción índices | ~2.0 s |
| Búsqueda (promedio) | ~1-5 ms |
| Toggle like/watchlist | ~0 ms |

#### Mejoras Posibles
1. Paralelizar construcción de índices (actualmente secuencial)
2. Usar punteros inteligentes en lugar de punteros crudos
3. Cachear búsquedas frecuentes
4. Implementar índices invertidos más eficientes

### Proceso de Desarrollo
#### Metodología
- División de trabajo por componentes
- Revisión de código entre compañeros

#### Herramientas Usadas
- **Editor**: VS Code y CLion
- **Compilador**: g++ (GCC 11+)
- **Control de versiones**: Git + GitHub
- **Build system**: CMake

## Contribución
1. Fork el repositorio
2. Crea una rama para tu feature (`git checkout -b feature/nueva-feature`)
3. Commit tus cambios (`git commit -m 'Agrega nueva feature'`)
4. Push a la rama (`git push origin feature/nueva-feature`)
5. Abre un Pull Request


---
<p align="center">Desarrollado con ❤️ por el equipo de RottenPotatoes</p>
