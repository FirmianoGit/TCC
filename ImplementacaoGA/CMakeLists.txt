cmake_minimum_required(VERSION 3.20)
project(scheduling_genetic_algorithm)

set(CMAKE_CXX_STANDARD 20)

add_executable(scheduling_genetic_algorithm
        main.cpp
        scheduling_ga.cpp
        "AlgoritmoGenetico/genetic_algorithm.cpp"
)
