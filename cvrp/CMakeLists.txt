cmake_minimum_required(VERSION 3.16)

project(cvrp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Opção para ativar PGO manualmente
option(ENABLE_PGO_GENERATE "Build with profile generation" OFF)
option(ENABLE_PGO_USE "Build with profile use" OFF)

add_executable(cvrp
    src/main.cpp
    src/core/Solution.cpp
    src/utils/parser.cpp
    src/utils/nearestNeighbor.cpp
    src/utils/CWSavings.cpp
    src/neighborhoods/2opt.cpp
    src/neighborhoods/relocate.cpp
    src/neighborhoods/swap.cpp
    src/neighborhoods/crossE.cpp
    src/metaheuristicas/SA.cpp
    src/metaheuristicas/ga.cpp
    src/metaheuristicas/BRKGA.cpp
    src/metaheuristicas/vns.cpp
    src/metaheuristicas/ils.cpp
    src/metaheuristicas/grasp.cpp
)

target_include_directories(cvrp PRIVATE include)

set_target_properties(cvrp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cvrp PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wpedantic
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    # Debug
    target_compile_options(cvrp PRIVATE
        $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
    )

    # Release
    target_compile_options(cvrp PRIVATE
        $<$<CONFIG:Release>:
            -O3
            -march=native
            -mtune=native
            -fomit-frame-pointer
            -ffast-math
            -funroll-loops
            -fstrict-aliasing
        >
    )

    target_compile_definitions(cvrp PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
    )

    # RelWithDebInfo: útil para perf com algum debug
    target_compile_options(cvrp PRIVATE
        $<$<CONFIG:RelWithDebInfo>:
            -O3
            -g
            -march=native
            -mtune=native
            -fomit-frame-pointer
            -ffast-math
            -funroll-loops
            -fstrict-aliasing
        >
    )

    target_compile_definitions(cvrp PRIVATE
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    )

endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported)
    set_property(TARGET cvrp PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET cvrp PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
else()
    message(WARNING "IPO/LTO não suportado: ${ipo_error}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(ENABLE_PGO_GENERATE AND ENABLE_PGO_USE)
        message(FATAL_ERROR "Ative apenas uma entre ENABLE_PGO_GENERATE e ENABLE_PGO_USE.")
    endif()

    if(ENABLE_PGO_GENERATE)
        target_compile_options(cvrp PRIVATE -fprofile-generate)
        target_link_options(cvrp PRIVATE -fprofile-generate)
        message(STATUS "PGO generate ativado")
    endif()

    if(ENABLE_PGO_USE)
        target_compile_options(cvrp PRIVATE -fprofile-use -fprofile-correction)
        target_link_options(cvrp PRIVATE -fprofile-use -fprofile-correction)
        message(STATUS "PGO use ativado")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(ENABLE_PGO_GENERATE AND ENABLE_PGO_USE)
        message(FATAL_ERROR "Ative apenas uma entre ENABLE_PGO_GENERATE e ENABLE_PGO_USE.")
    endif()

    if(ENABLE_PGO_GENERATE)
        target_compile_options(cvrp PRIVATE -fprofile-instr-generate)
        target_link_options(cvrp PRIVATE -fprofile-instr-generate)
        message(STATUS "PGO generate ativado (Clang)")
    endif()

    if(ENABLE_PGO_USE)
        target_compile_options(cvrp PRIVATE -fprofile-instr-use=default.profdata)
        target_link_options(cvrp PRIVATE -fprofile-instr-use=default.profdata)
        message(STATUS "PGO use ativado (Clang)")
    endif()
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
