cmake_minimum_required(VERSION 3.29)
project(RottenPotatoes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Incluir los directorios de encabezados
include_directories(csvreader)
include_directories(movie)

# Recopilar archivos fuente
file(GLOB_RECURSE SOURCES "csvreader/*.cpp" "movie/*.cpp")

# Crear el ejecutable
add_executable(${PROJECT_NAME} main.cpp ${SOURCES}
        index/NGramTrie.h
        engine/SearchEngine.h
        engine/SearchEngine.cpp)