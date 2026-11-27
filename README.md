# La Scossa

Applicazione web per condurre dal vivo **La Scossa**, un quiz con 8 modalità di gioco. Sostituisce il PowerPoint usato finora.

Gira su un solo portatile collegato al proiettore e ha due schermate:

- **Plancia**, sul monitor del portatile: i conduttori vedono domande, soluzioni e stato del gioco, e lo comandano con i pulsanti.
- **Pubblico**, a schermo intero sul proiettore: mostra solo ciò che la sala può vedere. Le soluzioni non arrivano mai a questa schermata.

Tutta la logica sta in un server C++20 (in un container Docker) che possiede lo stato della partita e lo fa evolvere con macchine a stati. L'interfaccia è in React.

## Documenti

| Documento | Contenuto |
| --- | --- |
| [Regole delle modalità](MODALITA'%201%20-%20Estas%20o%20non%20Estas.md) | Le regole delle 8 modalità, come le descrivono i conduttori |
| [Analisi dei requisiti](La%20Scossa%20analisi%20dei%20requisiti.md) | Attori, requisiti comuni e per modalità, rischi, domande aperte, fasi di rilascio |
| [Analisi del problema e architettura](La%20Scossa%20analisi%20del%20problema%20e%20architettura.md) | Stack scelto, motore a stati, protocollo client-server, dati, persistenza |
| [core/README.md](core/README.md) | Dettaglio delle macchine a stati e delle viste |
| [CLAUDE.md](CLAUDE.md) e [.claude/](.claude/) | Regole e procedure per lavorare al progetto con Claude Code |

## Stato

| Parte | Stato |
| --- | --- |
| Macchine a stati: serata, ciclo della domanda, partita (registro, annulla, replay) | Fatto, con test |
| Modalità 1, Estas o non Estas | Fatto, con test |
| Modalità 2, ESA (indizi in sequenza, timer di gioco) | Fatto, con test ed e2e |
| Proiezioni plancia/pubblico ed eventi ammessi | Fatto, con test |
| Server HTTP + WebSocket (Crow), caricamento CSV e config, registro su file con ripresa dopo crash | Fatto, con test ed e2e |
| Plancia, schermo pubblico e pagina di debug (React + Vite) | Prima versione |
| Dockerfile e docker-compose | Fatto: build con test nel container, immagine di 80 MB |
| Modalità 3–8 | Da fare (Duelli: regole rimandate) |
| Timer di gioco (RC-05): servizio nel server, pausa generale, annulla e ripresa in pausa | Fatto, usato da ESA |
| Timer liberi creati dalla plancia (RC-05) | Da fare |
| Schema JSON con generatore dei tipi C++/TypeScript | Da fare |

## Provare la plancia

Il modo più semplice è Docker, che non richiede altro installato:

```sh
docker compose up -d --build
```

Poi apri **<http://localhost:8080/plancia>** e, in un'altra finestra, **<http://localhost:8080/pubblico>**
(sul proiettore: schermo intero con F11). Dopo aver cambiato il codice, rilancia lo stesso comando;
se cambi solo i CSV o `config.json` basta `docker compose restart`. Per fermare: `docker compose down`.

### Senza Docker

Servono g++ (o Clang) con C++20, CMake 3.20+, Ninja e Node 22.

```sh
# 1. interfaccia
cd web && npm install && npm run build && cd ..

# 2. server e test
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

# 3. avvio
./build/lascossa-server --dati dati --web web/dist
```

Gli indirizzi sono gli stessi. La pagina **/debug** mostra registro, rifiuti e transizioni.

Il server riprende da solo l'ultima partita non chiusa: per ripartire da zero usa `--nuova`, oppure
in plancia "Chiudi serata" e poi "Nuova partita". Le domande sono in `dati/contenuti/mod1_estas.csv`
(si modificano con Excel, salvando come "CSV UTF-8"), i parametri in `dati/config.json`.

In sviluppo dell'interfaccia: avvia il server, poi `cd web && npm run dev` e apri <http://localhost:5173/plancia>.

## Struttura

```text
CMakeLists.txt       build di core, server e test
core/                macchine a stati e viste, C++ senza dipendenze esterne
server/src/          dati (CSV, config), registro su file, protocollo (regia), main con Crow
server/test/         test doctest del server ed e2e.mjs
web/                 React + Vite + TypeScript: /plancia, /pubblico, /debug
dati/                config.json e contenuti/*.csv; partite/ contiene i registri delle serate
third_party/         doctest, Crow, Asio, nlohmann/json (header-only)
Dockerfile           build multi-stadio: Node per React, g++ per il server, immagine finale minima
```

## Licenza

Apache 2.0, vedi [LICENSE](LICENSE). Le librerie in `third_party/` hanno le proprie licenze (MIT, BSD-3, Boost).
