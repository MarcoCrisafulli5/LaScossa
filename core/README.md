# La Scossa – nucleo delle macchine a stati (C++20)

Prima parte del server: le macchine a stati, senza rete né interfaccia.
Nessuna dipendenza esterna tranne `doctest` (header-only, solo per i test).

## Compilare e testare

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/lascossa_test
```

Compilazione con `-Wall -Wextra -Wpedantic -Wswitch-enum -Werror`: aggiungere uno stato o un
evento senza gestirlo in ogni `switch` blocca la build.

## Struttura

| File | Contenuto |
| --- | --- |
| `core/include/lascossa/eventi.hpp` | `TipoEvento`, `IdModalita`, `Evento` (tipo, argomento, ora) |
| `core/include/lascossa/esito.hpp` | `Esito` (accettato o rifiutato con motivo), `Traccia` delle transizioni |
| `core/include/lascossa/ciclo_domanda.hpp` | Ciclo comune di una domanda a scelta |
| `core/include/lascossa/estas.hpp` | Modalità 1, Estas o non Estas |
| `core/include/lascossa/serata.hpp` | Macchina della serata, contiene la modalità attiva |
| `core/include/lascossa/partita.hpp` | Punto d'ingresso: registro eventi, annulla, replay |
| `core/include/lascossa/viste.hpp` | Proiezioni `VistaPlancia` / `VistaPubblico` ed eventi ammessi |

## Macchine

**Serata**: `Lobby` → `SceltaModalita` ⇄ `InGioco` → `Chiusa`.
Pausa generale, blackout e riquadro punteggio (`MOSTRA_RIQUADRO`, `NASCONDI_RIQUADRO`, `AGGIORNA_RIQUADRO`)
valgono in ogni stato aperto; in pausa gli eventi di gioco sono rifiutati. `RICARICA_CONTENUTI` è ammesso solo fuori dal gioco.
In `InGioco` gli eventi di gioco scendono alla modalità attiva.

**Domanda** (riusata dalle modalità a scelta): `Mostrata` → `Selezionata` (giallo) → `Giusta` (verde) / `Sbagliata` (rosso);
`RIVELA` porta a `Rivelata`. Le modalità a scelta usano `CONFERMA` (decide la macchina); `CONFERMA_GIUSTA` e
`CONFERMA_SBAGLIATA` sono per le modalità a risposta aperta (ESA, Centos, Duelli), dove giudica il conduttore.
In `Selezionata` il conduttore può cambiare selezione prima di confermare. L'esito lo decide la macchina confrontando
con la soluzione. Con `riprova_dopo_errore` (modalità Date) l'opzione sbagliata resta rossa e si può scegliere un'altra.

**Estas**: `SceltaCategoria` → `OggettoInGioco` → `CategoriaFinita`.
Cambio categoria possibile a ogni oggetto, solo a esito dato; una categoria ripresa continua dall'oggetto successivo.

**Partita**: applica gli eventi alla serata, scrive nel registro solo quelli accettati, tiene i rifiutati per il debug.
`Annulla` ricostruisce lo stato rigiocando il registro senza l'ultimo evento (10.000 eventi: sotto 1 ms in Release).

## Viste

Il server non invia lo stato interno, ma due proiezioni calcolate da `viste.hpp`:

- **`VistaPubblico`**: categoria, oggetto, stato grafico delle opzioni (`neutra`, `selezionata`, `giusta`,
  `sbagliata`, `soluzione`), blackout, riquadro. Le sue struct non hanno campi per la soluzione: la
  soluzione arriva al pubblico solo come stato di un'opzione già giudicata o rivelata.
- **`VistaPlancia`**: tutto, comprese soluzione, categorie con oggetti giocati, turno, modalità
  disponibili, ultimi eventi, rifiutati e transizioni per il debug.
- **`eventi_ammessi()`**: prova ogni evento (con gli argomenti possibili) su una **copia** della
  serata (`Serata(const Serata&)` clona la modalità con `Modalita::clona()`). La plancia abilita
  solo i pulsanti di questi eventi, senza logica di gioco nel browser.

`Serata::disponibile(IdModalita)` dice quali modalità hanno una macchina implementata;
`tipo_da_nome("SELEZIONA")` converte i nomi del protocollo; `kTuttiGliEventi` e `kTutteLeModalita`
elencano tutti i valori degli enum (un test verifica che siano completi).

## Ipotesi da confermare

- Estas: dopo una risposta sbagliata il turno passa al concorrente successivo; dopo una giusta resta.
- Estas: opzione 0 = Estas, 1 = Non Estas.
- Le macchine non leggono l'orologio: l'ora arriva dentro l'evento.

## Prossimi passi

Servizio timer (dopo l'analisi di RC-05), altre modalità (vedi lo skill `.claude/skills/nuova-modalita`),
schema JSON con generatore dei tipi C++ e TypeScript.
