# Versão mínima do CMake e nome do projeto
cmake_minimum_required(VERSION 3.10)
project(ImplementacaoPSO)

# Definir padrão do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adicionar flags de compilação
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Incluir diretórios
include_directories(${CMAKE_SOURCE_DIR})

# Arquivos fonte
set(PSO_SOURCES
        "AlgoritmoPSO/scheduling_pso.cpp"
        ModeloProblema.cpp
        main.cpp
)

set(PSO_HEADERS
        "AlgoritmoPSO/scheduling_pso.h"
        ModeloProblema.cpp
        ModeloProblema.h
)

# Criar executável
add_executable(scheduling_pso ${PSO_SOURCES} ${PSO_HEADERS})

# Linking
if(UNIX)
    target_link_libraries(scheduling_pso m)
endif()

# Mensagem de sucesso
message(STATUS "PSO Scheduling project configured successfully!")
message(STATUS "Build with: cmake --build .")
message(STATUS "Run with: ./scheduling_pso [instance_file] [output_file] [generations] [population]")
