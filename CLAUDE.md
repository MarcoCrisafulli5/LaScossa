# La Scossa

Applicazione web per condurre dal vivo il quiz **La Scossa** (8 modalità), al posto di un PowerPoint.
Un portatile collegato al proiettore: la **plancia** sul monitor per i conduttori, lo schermo
**pubblico** a schermo intero sul proiettore. Server C++20 autoritativo in Docker, interfaccia React.

Lingua del progetto: **italiano** per codice (identificatori, commenti), documenti, messaggi e commit.

## Fonti di verità

Prima di decisioni architetturali o di comportamento di gioco, leggi il documento pertinente.
Se una richiesta contraddice questi documenti, dillo e chiedi prima di procedere.

| Documento | Cosa decide |
| --- | --- |
| [La Scossa analisi dei requisiti.md](La%20Scossa%20analisi%20dei%20requisiti.md) | requisiti RC-xx, regola giallo→verde/rosso, formato CSV, rischi, domande aperte, fasi |
| [La Scossa analisi del problema e architettura.md](La%20Scossa%20analisi%20del%20problema%20e%20architettura.md) | stack, motore FSM, protocollo, dati, persistenza, struttura cartelle |
| [MODALITA' 1 - Estas o non Estas.md](MODALITA'%201%20-%20Estas%20o%20non%20Estas.md) | regole delle 8 modalità scritte dai conduttori (la fonte primaria del gioco) |
| [core/README.md](core/README.md) | macchine a stati implementate e ipotesi da confermare |

I due documenti di analisi sono esportazioni dei Claude Docs dell'utente
([requisiti](https://claude.ai/artifact/1tp7f5uQp7v87fpr1Kq6ji),
[architettura](https://claude.ai/artifact/486g2RUELqdeoJSAnC56N2)), che contengono anche i diagrammi:
se l'utente aggiorna un documento là, la copia nel repo va riesportata.

Regole dettagliate in `.claude/rules/` (architettura e dominio sempre; C++, web e dati per percorso).

## Stato (aggiornare quando cambia)

- Fatto: core FSM (serata, ciclo domanda, Estas, partita con registro/annulla/replay), proiezioni
  plancia/pubblico ed eventi ammessi, server Crow + WebSocket, CSV/config, registro `.jsonl` con
  ripresa dopo crash, plancia/pubblico/debug in React, Dockerfile.
- Da fare: modalità 2–7, duelli come seconda partita (RC-15…20, punti aperti in sezione 5.1 dei
  requisiti), servizio timer (RC-05, analizzato il 27/11), generatore dei tipi dallo
  schema JSON, `RICARICA_CONTENUTI` a caldo, scorciatoie da tastiera, suoni, grafica definitiva.

## Comandi

```sh
# Tutto in Docker (build web + C++ con -Werror + ctest, poi avvio su :8080): non serve altro installato
docker compose up -d --build

# C++ (core + server + test). Su Windows: g++ di MSYS2 in C:/msys64/mingw64/bin, CMake e Ninja
# vanno installati (pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja).
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

# Web
cd web && npm install && npm run build      # tsc --noEmit + vite build -> web/dist
cd web && npm run dev                       # :5173, inoltra /ws al server su :8080

# Avvio
./build/lascossa-server --dati dati --web web/dist          # riprende l'ultima partita non chiusa
./build/lascossa-server --dati dati --web web/dist --nuova  # partita nuova
docker compose up                                           # come in sala

# End-to-end (server avviato con --nuova su una COPIA di dati/)
node server/test/e2e.mjs 8099
```

## Struttura

```text
core/        libreria C++ senza dipendenze: eventi, FSM (serata, ciclo_domanda, estas), partita, viste
server/src/  csv, dati (config + CSV), registro_file (.jsonl), json_viste, regia (protocollo), main (Crow)
server/test/ test doctest della regia e dei dati; e2e.mjs
web/src/     protocollo.ts (tipi dei messaggi), connessione.ts, plancia/, pubblico/, debug/
dati/        config.json, contenuti/*.csv, partite/ (registri, ignorati da git)
third_party/ doctest, Crow, Asio standalone, nlohmann/json (header-only, versionati)
VERSION      versione corrente (SemVer); CHANGELOG.md la storia delle modifiche
```

## Come lavorare qui

- Aggiungere una modalità: segui lo skill `nuova-modalita`.
- Prima di dire che una modifica è finita: skill `verifica` (build con -Werror, test C++, typecheck
  e build web, e2e se tocchi server o protocollo).
- Una modifica al protocollo tocca sempre **tre** posti: `core/include/lascossa/viste.hpp`,
  `server/src/json_viste.cpp`, `web/src/protocollo.ts`.
- Non introdurre librerie nuove senza chiedere: il minimo di dipendenze è una decisione del 24/11.
- **Changelog (deciso il 27/11)**: ogni modifica aggiunge una riga in `## [Non rilasciato]` di
  `CHANGELOG.md`, scritta per chi usa il gioco (cosa cambia in sala), non per chi legge il codice.
- **Versione**: unica fonte il file `VERSION` (SemVer, partita da 0.1.0); il tag in
  `docker-compose.yml` va tenuto uguale. Rilascio = aggiornare `VERSION` e il tag, dare numero e
  data alla sezione "Non rilasciato", e dopo il commit (solo se l'utente lo chiede) `git tag vX.Y.Z`.
  Una modalità nuova alza la minore (0.2.0); una correzione alza la patch (0.1.1).
