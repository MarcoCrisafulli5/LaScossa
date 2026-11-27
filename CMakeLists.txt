cmake_minimum_required(VERSION 3.20)
# Versione unica del progetto nel file VERSION (storia delle modifiche in CHANGELOG.md).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LASCOSSA_VERSIONE LIMIT_COUNT 1)
string(STRIP "${LASCOSSA_VERSIONE}" LASCOSSA_VERSIONE)  # su Windows il checkout può aggiungere \r
project(lascossa VERSION ${LASCOSSA_VERSIONE} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ogni switch su un enum deve elencare tutti i casi: uno stato o un evento dimenticato blocca la build.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wswitch-enum -Werror)
endif()

option(LASCOSSA_SERVER "Compila anche il server (Crow, nlohmann/json)" ON)

# --- core: macchine a stati e proiezioni, senza dipendenze esterne ---
add_library(lascossa_core
    core/src/eventi.cpp
    core/src/ciclo_domanda.cpp
    core/src/estas.cpp
    core/src/serata.cpp
    core/src/partita.cpp
    core/src/viste.cpp
)
target_include_directories(lascossa_core PUBLIC core/include)

# --- test del core (doctest, header-only) ---
enable_testing()
add_executable(lascossa_test
    core/test/main.cpp
    core/test/test_ciclo_domanda.cpp
    core/test/test_estas.cpp
    core/test/test_serata_partita.cpp
    core/test/test_viste.cpp
)
target_link_libraries(lascossa_test PRIVATE lascossa_core)
target_include_directories(lascossa_test SYSTEM PRIVATE third_party)
add_test(NAME lascossa_test COMMAND lascossa_test)

if(LASCOSSA_SERVER)
    find_package(Threads REQUIRED)

    # --- dati e protocollo: CSV, config, registro su file, JSON, regia (senza rete) ---
    add_library(lascossa_regia
        server/src/csv.cpp
        server/src/dati.cpp
        server/src/registro_file.cpp
        server/src/json_viste.cpp
        server/src/regia.cpp
    )
    target_include_directories(lascossa_regia PUBLIC server/src)
    target_include_directories(lascossa_regia SYSTEM PUBLIC third_party)
    target_link_libraries(lascossa_regia PUBLIC lascossa_core)

    # --- eseguibile: Crow (HTTP + WebSocket) su Asio standalone ---
    add_executable(lascossa-server server/src/main.cpp)
    target_include_directories(lascossa-server SYSTEM PRIVATE third_party/crow/include third_party/asio/include)
    target_compile_definitions(lascossa-server PRIVATE ASIO_STANDALONE LASCOSSA_VERSIONE="${PROJECT_VERSION}")
    target_link_libraries(lascossa-server PRIVATE lascossa_regia Threads::Threads)
    if(WIN32)
        target_compile_definitions(lascossa-server PRIVATE _WIN32_WINNT=0x0A00)
        target_link_libraries(lascossa-server PRIVATE ws2_32 mswsock)
    endif()

    # --- test del server ---
    add_executable(lascossa_server_test
        server/test/main.cpp
        server/test/test_dati.cpp
        server/test/test_registro.cpp
        server/test/test_regia.cpp
    )
    target_link_libraries(lascossa_server_test PRIVATE lascossa_regia)
    add_test(NAME lascossa_server_test COMMAND lascossa_server_test)
endif()
