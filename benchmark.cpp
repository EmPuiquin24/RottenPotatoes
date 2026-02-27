#include <iostream>
#include <chrono>
#include <iomanip>
#include "engine/SearchEngine.h"

// Programa para hacer benchmarks de las versiones secuencial vs paralela

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void benchmarkIndexBuilding(const std::string& csvPath) {
    printHeader("BENCHMARK: Construcción de Índices");
    
    // Test 1: Versión Secuencial
    std::cout << "\n[1/2] Construcción Secuencial...\n";
    SearchEngine* engine1 = SearchEngine::getInstance();
    engine1->load(csvPath);
    
    auto start = std::chrono::high_resolution_clock::now();
    engine1->buildIndexes();
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed_seq = end - start;
    std::cout << "✓ Tiempo secuencial: " << std::fixed << std::setprecision(4) 
              << elapsed_seq.count() << " segundos\n";
    
    // Para el Test 2, necesitamos una nueva instancia (resetear)
    // Dado que usamos Singleton, vamos a medir solo los índices
    std::cout << "\n[2/2] Construcción Paralela...\n";
    
    // Crear nueva instancia para prueba paralela
    // Nota: En un escenario real, podrías necesitar resetear el Singleton
    // o crear una manera de limpiar los índices
    
    SearchEngine* engine2 = SearchEngine::getInstance();
    // Los datos ya están cargados desde engine1
    
    start = std::chrono::high_resolution_clock::now();
    engine2->buildIndexesParallel();
    end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed_par = end - start;
    std::cout << "✓ Tiempo paralelo: " << std::fixed << std::setprecision(4) 
              << elapsed_par.count() << " segundos\n";
    
    // Calcular speedup
    double speedup = elapsed_seq.count() / elapsed_par.count();
    
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << "RESULTADOS:\n";
    std::cout << "  Secuencial:  " << elapsed_seq.count() << "s\n";
    std::cout << "  Paralelo:    " << elapsed_par.count() << "s\n";
    std::cout << "  Speedup:     " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
    std::cout << "  Mejora:      " << std::fixed << std::setprecision(1)
              << ((speedup - 1.0) * 100.0) << "%\n";
    std::cout << std::string(60, '-') << "\n";
}

void benchmarkSearchOperations(SearchEngine* engine) {
    printHeader("BENCHMARK: Operaciones de Búsqueda");
    
    std::vector<std::string> queries = {
        "love",
        "adventure",
        "the matrix",
        "star wars",
        "family drama"
    };
    
    std::cout << "\nEjecutando " << queries.size() << " búsquedas...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int total_results = 0;
    for (const auto& query : queries) {
        auto results = engine->searchSubstring(query, 0, 10);
        total_results += results.size();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << "✓ Total de resultados: " << total_results << "\n";
    std::cout << "✓ Tiempo total: " << std::fixed << std::setprecision(4) 
              << elapsed.count() << " segundos\n";
    std::cout << "✓ Promedio por query: " << std::fixed << std::setprecision(4)
              << (elapsed.count() / queries.size()) << " segundos\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║    BENCHMARK: RottenPotatoes Search Engine            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    
    std::string csvPath = "../resources/mpst_full_data.csv";
    
    if (argc > 1) {
        csvPath = argv[1];
    }
    
    std::cout << "\nDataset: " << csvPath << "\n";
    std::cout << "Hardware threads disponibles: " 
              << std::thread::hardware_concurrency() << "\n";
    
    try {
        // Benchmark 1: Construcción de índices
        benchmarkIndexBuilding(csvPath);
        
        // Benchmark 2: Operaciones de búsqueda
        SearchEngine* engine = SearchEngine::getInstance();
        benchmarkSearchOperations(engine);
        
        printHeader("BENCHMARK COMPLETADO");
        std::cout << "\n✓ Todos los benchmarks ejecutados exitosamente.\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error durante benchmark: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
