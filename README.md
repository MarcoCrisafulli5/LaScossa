# La Scossa

Applicazione web per condurre dal vivo **La Scossa**, un quiz con 8 modalità di gioco. Sostituisce il PowerPoint usato finora.

Gira su un solo portatile collegato al proiettore e ha due schermate:

- **Plancia**, sul monitor del portatile: i conduttori vedono domande, soluzioni e stato del gioco, e lo comandano con i pulsanti.
- **Pubblico**, a schermo intero sul proiettore: mostra solo ciò che la sala può vedere. Le soluzioni non arrivano mai a questa schermata.

Tutta la logica sta in un server C++20 in un container Docker, che possiede lo stato della partita e lo fa evolvere con macchine a stati. L'interfaccia sarà in React.

## Documenti

| Documento | Contenuto |
| --- | --- |
| [Regole delle modalità](MODALITA'%201%20-%20Estas%20o%20non%20Estas.md) | Le regole delle 8 modalità, come le descrivono i conduttori |
| [Analisi dei requisiti](La%20Scossa%20analisi%20dei%20requisiti.md) | Attori, requisiti comuni e per modalità, rischi, domande aperte, fasi di rilascio |
| [Analisi del problema e architettura](La%20Scossa%20analisi%20del%20problema%20e%20architettura.md) | Stack scelto, motore a stati, protocollo client-server, dati, persistenza |
| [core/README.md](core/README.md) | Dettaglio delle macchine a stati già implementate |

## Stato

| Parte | Stato |
| --- | --- |
| Macchine a stati: serata, ciclo della domanda, partita (registro, annulla, replay) | Fatto, con test |
| Modalità 1, Estas o non Estas | Fatto, con test |
| Modalità 2–8 | Da fare |
| Proiezioni plancia/pubblico, timer, caricamento CSV | Da fare |
| Server HTTP + WebSocket (Crow) | Da fare |
| Interfaccia React + Vite | Da fare |
| Immagine Docker | Da fare |

## Struttura

```
CMakeLists.txt     build del core e dei test
core/
  include/lascossa/  header: eventi, esito, ciclo della domanda, modalità, serata, partita
  src/               implementazioni
  test/              test doctest delle macchine a stati
third_party/
  doctest/           doctest 2.4.11 (header-only, solo per i test)
```

Le cartelle `server/`, `web/`, `schema/` e `dati/` previste dall'architettura arriveranno con le fasi successive.

## Compilare e testare

Servono un compilatore C++20 (GCC o Clang) e CMake 3.20 o successivo.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/lascossa_test
```

La build usa `-Wall -Wextra -Wpedantic -Wswitch-enum -Werror`. Se aggiungi uno stato o un evento e non lo gestisci in ogni `switch`, la compilazione si blocca.

## Licenza

Apache 2.0, vedi [LICENSE](LICENSE).
