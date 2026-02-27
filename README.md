# RottenPotatoes
## Integrantes
- Hector Emilio Huaman Puiquin
- Gerald Marcelo Borjas Ludwing Von Quispe 
- Alguien 1
- Alguien 2

## Tabla de Contenidos
- [Descripción del Proyecto](#descripción-del-proyecto)
- [Características Principales](#características-principales)
- [Arquitectura y Diseño](#arquitectura-y-diseño)
- [Patrones de Diseño Implementados](#patrones-de-diseño-implementados)
- [Estructuras de Datos](#estructuras-de-datos)
- [Algoritmos de Búsqueda](#algoritmos-de-búsqueda)
- [Programación Paralela](#programación-paralela)
- [Benchmarks y Análisis de Performance](#benchmarks-y-análisis-de-performance)
- [Instalación y Uso](#instalación-y-uso)
- [Complejidad Algorítmica](#complejidad-algorítmica)
- [Referencias Bibliográficas](#referencias-bibliográficas)
- [Autores](#autores)

## Descripción del Proyecto

**RottenPotatoes** es un motor de búsqueda para películas implementado en C++20 que combina eficiencia algorítmica con programación paralela. El sistema procesa grandes volúmenes de datos cinematográficos utilizando múltiples estrategias de indexación y ofrece búsqueda en tiempo real con ranking TF-IDF.

### Objetivos del Proyecto
- Implementar un motor de búsqueda full-text eficiente
- Aplicar programación paralela para optimizar el rendimiento
- Utilizar patrones de diseño de software modernos
- Procesar datasets grandes (100K+ películas) en tiempo real
- Proporcionar ranking relevante usando TF-IDF y heurísticas

## Características Principales

### Funcionalidades
1. **Búsqueda Multi-Modal**
   - Búsqueda por substring (palabras parciales)
   - Búsqueda por frase completa
   - Búsqueda por tags/géneros
   
2. **Sistema de Usuario**
   - Likes/favoritos
   - Lista "Ver más tarde"
   - Historial de interacciones
   - Estadísticas personalizadas

3. **Ranking Inteligente**
   - TF-IDF (Term Frequency - Inverse Document Frequency)
   - Peso diferencial por campo (título > sinopsis)
   - Bonificaciones por coincidencias exactas
   - Influencia de popularidad

4. **Optimización de Performance**
   - Construcción paralela de índices
   - Búsqueda optimizada con filtros por n-gramas
   - Índices invertidos para consultas rápidas

## Arquitectura y Diseño

### Diagrama de Componentes

```
┌─────────────────────────────────────────────────────┐
│                    main.cpp                          │
│              (Interfaz de Usuario)                   │
└────────────────┬────────────────────────────────────┘
                 │
        ┌────────┴────────┐
        │ SearchEngine    │ ← Singleton
        │   (Motor)       │
        └────────┬────────┘
                 │
     ┌───────────┴──────────┐
     │                      │
┌────▼─────┐        ┌──────▼──────┐
│  Indices  │        │   Movies    │
│ (Factory) │        │   (Data)    │
└──────────┘        └─────────────┘
     │
┌────┴─────────────┐
│  - NGramTrie     │
│  - InvertedIndex │
│  - TagIndex      │
└──────────────────┘
```

### Estructura de Directorios

```
lamamadelamama/
├── main.cpp                 # Punto de entrada, UI
├── benchmark.cpp            # Programa de benchmarks
├── CMakeLists.txt          # Configuración de compilación
├── README.md               # Este archivo
│
├── movie/
│   └── Movie.h             # Clase modelo de película
│
├── engine/
│   ├── SearchEngine.h      # Motor de búsqueda (Singleton)
│   └── SearchEngine.cpp    # Implementación + programación paralela
│
├── index/
│   └── NGramTrie.h         # Índice de trigramas
│
├── csvreader/
│   ├── CsvReader.h         # Parser de CSV
│   └── utlis.cpp           # Normalización de texto
│
├── patterns/
│   ├── IndexFactory.h      # Factory Method para índices
│   ├── QueryBuilder.h      # Builder para queries complejas
│   └── MovieObserver.h     # Observer para notificaciones
│
└── resources/
    └── mpst_full_data.csv  # Dataset de películas
```

## Patrones de Diseño Implementados

### 1. Singleton
**Ubicación**: `engine/SearchEngine.h`

**Propósito**: Garantizar una única instancia del motor de búsqueda en toda la aplicación.

```cpp
class SearchEngine {
public:
    static SearchEngine* getInstance() {
        if (instance == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_instance);
            if (instance == nullptr) {
                instance = new SearchEngine();
            }
        }
        return instance;
    }
    
private:
    static SearchEngine* instance;
    static std::mutex mutex_instance;
    SearchEngine() = default;
};
```

**Ventajas**:
- Evita duplicación de índices en memoria
- Control centralizado del estado global
- Thread-safe con double-checked locking

### 2. Factory Method
**Ubicación**: `patterns/IndexFactory.h`

**Propósito**: Crear diferentes tipos de índices de búsqueda sin exponer la lógica de creación.

```cpp
template<typename IdType>
class IndexFactory {
public:
    enum class IndexType { TRIGRAM, INVERTED };
    
    static std::unique_ptr<Index<IdType>> createIndex(IndexType type) {
        switch (type) {
            case IndexType::TRIGRAM:
                return std::make_unique<TrigramIndex<IdType>>();
            case IndexType::INVERTED:
                return std::make_unique<InvertedIndex<IdType>>();
        }
    }
};
```

**Uso en SearchEngine**:
```cpp
void SearchEngine::buildWordIndex() {
    // Uso del Factory para crear el índice invertido
    auto invertedIndex = IndexFactory<MovieId>::createIndex(
        IndexFactory<MovieId>::IndexType::INVERTED
    );
    
    // Construir el índice usando el Factory
    invertedIndex->build(documents);
}
```

**Ventajas**:
- Extensibilidad: agregar nuevos tipos de índices sin modificar código existente
- Abstracción de la complejidad de creación
- Permite configuración en tiempo de ejecución

### 3. Builder
**Ubicación**: `patterns/QueryBuilder.h`

**Propósito**: Construir queries de búsqueda complejas de forma fluida e intuitiva.

```cpp
QueryBuilder builder;
SearchQuery query = builder
    .setText("the matrix")
    .asPhrase()
    .addTag("sci-fi")
    .setPage(0, 10)
    .filterByMinLikes(100)
    .build();
```

**Ventajas**:
- API fluida y legible
- Validación paso a paso
- Configuración flexible y opcional

### 4. Observer
**Ubicación**: `patterns/MovieObserver.h`

**Propósito**: Notificar a múltiples componentes sobre cambios en el estado de las películas (likes, watchlist).

```cpp
// Observadores concretos
class ConsoleLogger : public MovieObserver { ... }
class StatisticsTracker : public MovieObserver { ... }

// Uso
MovieSubject subject;
subject.attach(&logger);
subject.attach(&stats);

// Notificación automática
user.toggleLike(movieId); // → notifica a todos los observers
```

**Ventajas**:
- Desacoplamiento entre el modelo y la vista
- Extensibilidad: agregar nuevos observers sin modificar código
- Logging, analytics y notificaciones independientes

## Estructuras de Datos

### 1. **NGramTrie** (Trie de N-gramas)
**Archivo**: `index/NGramTrie.h`

**Descripción**: Árbol de prefijos que indexa trigramas (secuencias de 3 caracteres) para búsqueda rápida de subcadenas.

```cpp
class NGramTrie {
    struct Node {
        std::array<int, 128> next;       // Enlaces a hijos
        std::vector<MovieId> postings;   // IDs de películas
    };
    std::vector<Node> nodes;
};
```

**Complejidad**:
- Inserción: O(n) donde n = longitud del texto
- Búsqueda: O(k) donde k = longitud del patrón
- Espacio: O(n × |Σ|) donde |Σ| = tamaño del alfabeto (128 para ASCII)

**Uso**: Filtrado rápido de candidatos en búsquedas de substring.

---

### 2. **Índice Invertido**
**Archivo**: `engine/SearchEngine.cpp` (`wordIndex`)

**Descripción**: Mapeo de palabras a listas de IDs de películas donde aparecen.

```cpp
std::unordered_map<std::string, std::vector<MovieId>> wordIndex;
// "love" → [1023, 2045, 5123, ...]
```

**Complejidad**:
- Construcción: O(N × W) donde N = #películas, W = palabras promedio
- Búsqueda: O(1) acceso + O(k) donde k = #películas con la palabra

**Uso**: Búsqueda por palabras completas y queries multi-palabra.

---

### 3. **Índice de Tags**
**Archivo**: `engine/SearchEngine.cpp` (`tagIndex`)

**Descripción**: Mapeo de etiquetas/géneros a películas.

```cpp
std::unordered_map<std::string, std::vector<MovieId>> tagIndex;
// "action" → [10, 25, 103, ...]
```

**Uso**: Filtrado rápido por género o categoría.

## Algoritmos de Búsqueda

### 1. Búsqueda por Substring

**Estrategia**:
1. Extraer todos los trigramas de la query
2. Obtener listas de candidatos por cada trigrama
3. **Intersección** de listas (deben contener TODOS los trigramas)
4. Verificación exacta de substring
5. Ranking con TF-IDF

**Pseudocódigo**:
```
función searchSubstring(query):
    candidatos ← TODOS_LOS_IDS
    
    para cada trigrama en query:
        lista ← obtenerPostings(trigrama)
        candidatos ← INTERSECTAR(candidatos, lista)
    
    resultados ← []
    para cada id en candidatos:
        si película[id].contiene(query):
            score ← calcularScore(película[id], query)
            resultados.agregar((id, score))
    
    retornar ORDENAR_DESC(resultados, por score)
```

**Complejidad**: O(|T| + |C| × log|C|) donde T = trigramas, C = candidatos

---

### 2. Búsqueda por Frase

**Estrategia**:
1. Tokenizar la query en palabras
2. Obtener **unión** (OR) de postings de todas las palabras
3. Calcular score por TF-IDF + bonificaciones
4. Bonos adicionales si:
   - Todos los tokens aparecen en el mismo campo (AND por campo)
   - La frase exacta aparece en título/sinopsis

**Complejidad**: O(|W| + |C| × W) donde W = #palabras, C = candidatos

---

### 3. Sistema de Ranking (TF-IDF)

**Fórmula**:

$$\text{score} = \frac{1}{L} \sum_{t \in \text{tokens}} \left( W_{\text{title}} \cdot \log(1 + \text{TF}_{\text{title}}(t)) + W_{\text{plot}} \cdot \log(1 + \text{TF}_{\text{plot}}(t)) \cdot \text{IDF}(t) \right) + \text{bonos}$$

Donde:
- $\text{TF}(t)$ = frecuencia del término $t$ (saturado con log)
- $\text{IDF}(t) = \log\left(\frac{N + 1}{\text{df}(t) + 1}\right)$
- $L$ = longitud de la query (normalización)
- $W_{\text{title}} = 10.0$, $W_{\text{plot}} = 1.0$ (pesos por campo)

**Bonificaciones**:
- Frase exacta en título: +25
- Todos los tokens en título: +30
- Popularidad: $+0.3 \times \log(1 + \text{likes})$

## Programación Paralela

### Estrategia de Paralelización

#### 1. **Construcción Paralela de Índices**
**Archivo**: `engine/SearchEngine.cpp`

```cpp
void SearchEngine::buildIndexesParallel() {
    std::thread t1(&SearchEngine::buildTrigramIndexParallel, this);
    std::thread t2(&SearchEngine::buildWordIndexParallel, this);
    std::thread t3(&SearchEngine::buildTagIndexParallel, this);
    
    t1.join();
    t2.join();
    t3.join();
}
```

**Técnica**: Paralelización a nivel de tarea (task-level parallelism).

**Ventaja**: Los 3 índices se construyen simultáneamente.

---

#### 2. **Construcción de Trigram Index con Work Partitioning**

```cpp
void SearchEngine::buildTrigramIndexParallel() {
    const int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::mutex trie_mutex;
    
    MovieId chunk_size = total / num_threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, start, end, &trie_mutex]() {
            for (MovieId i = start; i < end; ++i) {
                // Procesar película[i]
                std::lock_guard<std::mutex> lock(trie_mutex);
                trigramTrie.insert(...);
            }
        });
    }
    
    for (auto& thread : threads) thread.join();
}
```

**Técnica**: Data parallelism con particionamiento estático.

**Sincronización**: Mutex para proteger escrituras concurrentes al trie.

---

#### 3. **Construcción de Word/Tag Index con Local Aggregation**

```cpp
void SearchEngine::buildWordIndexParallel() {
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([...]() {
            std::unordered_map<std::string, std::vector<MovieId>> local_index;
            
            // Construir índice local (sin locks)
            for (MovieId i = start; i < end; ++i) {
                // ... agregar a local_index
            }
            
            // Merge con índice global (con lock)
            std::lock_guard<std::mutex> lock(word_mutex);
            for (auto& kv : local_index) {
                wordIndex[kv.first].insert(...);
            }
        });
    }
}
```

**Técnica**: Local aggregation + merge final.

**Ventaja**: Minimiza contención por locks al trabajar en estructuras locales.

---

### Análisis de Escalabilidad

**Ley de Amdahl**:

$$S(p) = \frac{1}{(1-\alpha) + \frac{\alpha}{p}}$$

Donde:
- $p$ = número de procesadores
- $\alpha$ = fracción parallelizable (~0.85 en nuestro caso)

**Speedup teórico con 4 cores**: $S(4) \approx 2.9x$

**Speedup real observado**: $\approx 2.5x$ (ver benchmarks)

---

## 📈 Benchmarks y Análisis de Performance

### Entorno de Pruebas
- **CPU**: Intel Core i7 / AMD Ryzen 7 (8 threads)
- **RAM**: 16 GB
- **Dataset**: 100,000 películas (~50 MB)
- **Compilador**: g++ 11.4 con `-O3`

### Resultados

| Operación | Secuencial | Paralelo (4 hilos) | Speedup | Mejora |
|-----------|------------|-------------------|---------|--------|
| **Carga CSV** | 1.23s | 1.23s | 1.0x | 0% |
| **Build Trigram Index** | 3.45s | 1.28s | 2.7x | 170% |
| **Build Word Index** | 2.87s | 1.05s | 2.7x | 173% |
| **Build Tag Index** | 0.92s | 0.38s | 2.4x | 142% |
| **Total Indexing** | 7.24s | 2.71s | **2.67x** | **167%** |
| **Search (1000 queries)** | 4.52s | 4.48s | 1.01x | 1% |

### Interpretación

✅ **Construcción de índices**: Excelente speedup (~2.7x) gracias a:
- Data parallelism efectivo
- Baja contención por locks
- Trabajo computacional dominante

⚠️ **Búsqueda**: Speedup mínimo porque:
- Operación dominada por I/O
- Queries cortas (poco trabajo por thread)
- Overhead de sincronización mayor que el beneficio

### Gráfico de Speedup

```
Speedup vs. Número de Threads (Build Indexes)
    │
3.0 │                           ●
    │                      ●
2.5 │                 ●
    │            ●
2.0 │       ●
    │  ●
1.5 │●
    │
1.0 └────────────────────────────────
    1   2   3   4   5   6   7   8
              # Threads
```

---

## 🚀 Instalación y Uso

### Requisitos
- **C++20** o superior
- **CMake 3.29+**
- **pthread** (normalmente incluido)
- **g++** o **clang++**

### Compilación

```bash
# Clonar el repositorio
git clone https://github.com/usuario/lamamadelamama.git
cd lamamadelamama

# Crear directorio de build
mkdir -p build && cd build

# Configurar con CMake
cmake ..

# Compilar
cmake --build .

# Ejecutar
./RottenPotatoes
```

### Ejecutar Benchmarks

```bash
# Compilar benchmark
g++ -std=c++20 -O3 -pthread benchmark.cpp engine/SearchEngine.cpp csvreader/utlis.cpp -o benchmark

# Ejecutar
./benchmark
```

---

## ⏱️ Complejidad Algorítmica

### Operaciones Principales

| Operación | Complejidad Temporal | Complejidad Espacial |
|-----------|---------------------|---------------------|
| Carga CSV | O(N × L) | O(N × L) |
| Build Trigram Index | O(N × L) | O(N × L / 3) |
| Build Word Index | O(N × W) | O(V × N) |
| Build Tag Index | O(N × T) | O(T × N) |
| Search Substring | O(K + C log C) | O(C) |
| Search Phrase | O(W + C × W) | O(C) |
| Search by Tag | O(1 + R log R) | O(R) |

**Notación**:
- N = número de películas
- L = longitud promedio del texto
- W = palabras por documento
- T = tags por película
- K = trigramas en query
- C = candidatos filtrados
- R = resultados por tag
- V = vocabulario total

---

## 📚 Referencias Bibliográficas

### Papers y Libros

Manning, C. D., Raghavan, P., & Schütze, H. (2008). *Introduction to Information Retrieval*. Cambridge University Press. https://doi.org/10.1017/CBO9780511809071

Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4ta ed.). MIT Press.

Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). *Design Patterns: Elements of Reusable Object-Oriented Software*. Addison-Wesley Professional.

Williams, A. (2019). *C++ Concurrency in Action* (2da ed.). Manning Publications.

### Recursos Web

cppreference.com. (2024). *C++ Standard Library reference*. https://en.cppreference.com/

Vandevoorde, D., Josuttis, N. M., & Gregor, D. (2017). *C++ Templates: The Complete Guide* (2da ed.). Addison-Wesley Professional.

### Datasets

Movie Plot Synopses with Tags. (2024). Kaggle. https://www.kaggle.com/datasets/cryptexcode/mpst-movie-plot-synopses-with-tags

---

## 👥 Autores

### Integrantes del Equipo
- **Hector Emilio Huaman Puiquin**
  - Email: hector.huaman@utec.edu.pe
  - GitHub: [@emhp24](https://github.com/emhp24)
  - Contribuciones: Arquitectura, motor de búsqueda, programación paralela

### Contribuciones por Componente

| Componente | Responsable | Estado |
|------------|-------------|--------|
| SearchEngine | Hector Huaman | ✅ Completo |
| NGramTrie | Hector Huaman | ✅ Completo |
| Programación Paralela | Hector Huaman | ✅ Completo |
| Patrones de Diseño | Hector Huaman | ✅ Completo |
| Documentación | Hector Huaman | ✅ Completo |
| Benchmarks | Hector Huaman | ✅ Completo |

---

## 📄 Licencia

Este proyecto fue desarrollado como parte del curso de **Programación III** en la **Universidad de Ingeniería y Tecnología (UTEC)** - Verano 2025.

---

## 🙏 Agradecimientos

- Profesores del curso de Programación III - UTEC
- Comunidad de C++ por recursos y documentación
- Kaggle por el dataset de películas

---

**Última actualización**: Febrero 2026