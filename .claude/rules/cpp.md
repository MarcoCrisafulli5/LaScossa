---
paths:
  - "core/**"
  - "server/**"
  - "CMakeLists.txt"
---

# Convenzioni C++

- C++20, compilato con `-Wall -Wextra -Wpedantic -Wswitch-enum -Werror` (GCC e Clang). Zero warning.
- Identificatori in italiano, `snake_case` per funzioni e variabili, `PascalCase` per tipi,
  `kCostante` per costanti, membri privati con `_` finale. Namespace `lascossa` (core) e
  `lascossa::server` (server).
- `switch` su un enum: elenca tutti i casi, niente `default`, così `-Wswitch-enum` segnala ciò che manca.
  Dopo lo switch, un `return` di sicurezza.
- Dentro una classe che ha un `nome(Stato)` statico, la funzione libera va qualificata:
  `lascossa::nome(IdModalita)`.
- **core/** non dipende da nulla di esterno (né JSON, né rete, né filesystem). Il JSON sta in
  `server/src/json_viste.cpp`; i file in `server/src/dati.cpp` e `registro_file.cpp`.
- Errori nei dati: `ErroreDati` con messaggio per chi prepara i contenuti: `"<file> riga N: <problema>"`.
- Librerie di terze parti in `third_party/`, incluse come `SYSTEM` (i loro warning non contano).
  Non aggiornarle né aggiungerne senza chiedere.

## Test

- doctest. Core: `core/test/`. Server senza rete: `server/test/` (usa `CartellaTemporanea` per i file).
- Ogni macchina a stati nuova: test che inviano sequenze di eventi e controllano stato, rifiuti
  (con `CHECK_FALSE`) e traccia. Ogni modalità: test della vista pubblico senza soluzione.
- Un nuovo file `.cpp` va aggiunto a `CMakeLists.txt`.

## Compilare su questa macchina (Windows)

g++ 14 di MSYS2 è in `C:/msys64/mingw64/bin`. Se CMake non è installato, compila a mano con gli
stessi flag (`-Icore/include -Iserver/src -isystem third_party`, per il server anche
`-isystem third_party/crow/include -isystem third_party/asio/include -DASIO_STANDALONE
-D_WIN32_WINNT=0x0A00 ... -lws2_32 -lmswsock`) mettendo l'output nella scratchpad, non nel repo.
