cmake_minimum_required(VERSION 3.20)
project(lascossa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ogni switch su un enum deve elencare tutti i casi: uno stato o un evento dimenticato blocca la build.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wswitch-enum -Werror)
endif()

# --- core: macchine a stati, senza dipendenze esterne ---
add_library(lascossa_core
    core/src/eventi.cpp
    core/src/ciclo_domanda.cpp
    core/src/estas.cpp
    core/src/serata.cpp
    core/src/partita.cpp
)
target_include_directories(lascossa_core PUBLIC core/include)

# --- test (doctest, header-only) ---
enable_testing()
add_executable(lascossa_test
    core/test/main.cpp
    core/test/test_ciclo_domanda.cpp
    core/test/test_estas.cpp
    core/test/test_serata_partita.cpp
)
target_link_libraries(lascossa_test PRIVATE lascossa_core)
target_include_directories(lascossa_test SYSTEM PRIVATE third_party)
add_test(NAME lascossa_test COMMAND lascossa_test)
