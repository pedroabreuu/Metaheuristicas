cmake_minimum_required(VERSION 3.15)
project(cvrp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(cvrp
    src/main.cpp
    src/parser.cpp
    src/Solution.cpp
    src/nearestNeighbor.cpp
    src/CWSavings.cpp
    src/2opt.cpp
    src/relocate.cpp
)

target_include_directories(cvrp PRIVATE include)
target_compile_options(cvrp PRIVATE -Wall -Wextra -g)
