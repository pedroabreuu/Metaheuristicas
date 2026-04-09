cmake_minimum_required(VERSION 3.15)
project(cvrp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(cvrp
    src/main.cpp
    src/core/Solution.cpp
    src/utils/parser.cpp
    src/utils/nearestNeighbor.cpp
    src/utils/CWSavings.cpp
    src/neighborhoods/2opt.cpp
    src/neighborhoods/relocate.cpp
    src/metaheuristics/SA.cpp
    src/metaheuristics/ga.cpp
    src/metaheuristics/BRKGA.cpp
)

target_include_directories(cvrp PRIVATE include)

target_compile_options(cvrp PRIVATE -Wall -Wextra)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(cvrp PRIVATE -O0 -g)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(cvrp PRIVATE
        -O3
        -march=native
        -funroll-loops
        -fomit-frame-pointer
    )

    target_link_options(cvrp PRIVATE -flto)
endif()