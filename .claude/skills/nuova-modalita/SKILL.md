---
name: nuova-modalita
description: Procedura per aggiungere una delle modalità di La Scossa (ESA, Date, La Scossa, Triello, Centos, Ghigliottina, Duelli) dal core C++ fino alla plancia e al pubblico. Usala quando l'utente chiede di implementare, aggiungere o abilitare una modalità di gioco.
---

# Aggiungere una modalità

Segui i passi nell'ordine. Estas (`core/src/estas.cpp`) è il modello da imitare.

## 0. Prima di scrivere codice

1. Leggi le regole della modalità in `MODALITA' 1 - Estas o non Estas.md` e la sua riga nella
   tabella della sezione 5 di `La Scossa analisi dei requisiti.md` (colonna "Da chiarire").
2. Se la colonna "Da chiarire" ha domande aperte che cambiano la macchina a stati, **chiedile
   all'utente** prima di procedere. Se la modalità usa timer, ricorda che l'analisi dei timer
   (RC-05) non è ancora fatta: chiedi come procedere.
3. **Duelli**: non seguire questa procedura così com'è. Il duello può avvenire in qualsiasi momento,
   con schermata pubblico e plancia dedicate (requisiti, sezione 8.1): serve prima un'analisi del
   problema con l'utente, perché cambia il modello "una modalità alla volta" della Serata.
4. Proponi all'utente il diagramma degli stati (stati, eventi, transizioni) in testo e fattelo approvare.

## 1. Core (`core/`, nessuna dipendenza esterna)

1. `include/lascossa/<modalita>.hpp` + `src/<modalita>.cpp`: classe `final` che deriva da `Modalita`.
   - `enum class Stato`, `static std::string_view nome(Stato)`.
   - `gestisci()` con uno `switch` sullo stato; un gestore `su_<stato>()` per stato con `switch` su
     **tutti** i `TipoEvento`, senza `default`.
   - Riusa `CicloDomanda` per le domande a scelta (con `riprova_dopo_errore` se l'errore non chiude
     la domanda, come in Date) e `Turni` per il turno.
   - `clona()` = `std::make_unique<Classe>(*this)`: la classe deve restare copiabile.
   - Getter di sola lettura per tutto ciò che le viste devono mostrare.
2. Se servono eventi nuovi: aggiungili a `TipoEvento` **e** a `kTuttiGliEventi` in `eventi.hpp`, e
   a `nome()` in `eventi.cpp`. La build segnalerà ogni `switch` da aggiornare: aggiornali tutti.
3. Contenuti: aggiungi le struct (es. `DomandaTriello`) e un campo in `Contenuti` (`serata.hpp`).
4. `Serata::disponibile()` → `true`; `Serata::crea_modalita()` → costruisci la modalità.
5. Viste (`viste.hpp` / `viste.cpp`):
   - `XxxPlancia` con soluzione, e `XxxPubblico` **senza alcun campo per la soluzione**.
   - Campo `std::optional<XxxPlancia>` in `VistaPlancia` e `std::optional<XxxPubblico>` in `VistaPubblico`.
   - In `argomenti()` gli argomenti possibili per gli eventi con `arg` di questa modalità.
6. Test in `core/test/test_<modalita>.cpp`, aggiunto a `CMakeLists.txt`:
   sequenze di eventi, rifiuti, turni, e la vista pubblico che non mostra la soluzione prima del giudizio.

## 2. Server (`server/src/`)

1. `dati.cpp`: `leggi_<modalita>()` dal CSV della sezione 4.1 dei requisiti (`.claude/rules/dati.md`),
   errori `ErroreDati` con file e riga; chiamalo in `carica_dati()`.
2. `json_viste.cpp`: serializza le due viste nuove.
3. Test in `server/test/test_dati.cpp` (CSV validi e non) e in `test_regia.cpp`: il JSON del
   pubblico non contiene la soluzione.
4. Parametri (tempi, numero di indizi…) in `dati/config.json` con default, letti in `leggi_config()`.
5. CSV di esempio in `dati/contenuti/`, con contenuti **corretti nel merito**.

## 3. Web (`web/src/`)

1. `protocollo.ts`: tipi `XxxPlancia` / `XxxPubblico` identici al JSON.
2. `plancia/XxxPlancia.tsx`: pulsanti abilitati solo con `ammesso(...)`; "segna" separato da "mostra".
3. `pubblico/Pubblico.tsx`: ramo per la modalità; misure in `cqw`, colori da variabili CSS.
4. `plancia/Plancia.tsx` → `InGioco`: mostra il componente nuovo.

## 4. Chiusura

1. Esegui lo skill `verifica` (build, test C++, web, e2e).
2. Aggiorna `core/README.md` (macchine, ipotesi), la sezione "Stato" di `CLAUDE.md`,
   la tabella "Modalità implementate" in `.claude/rules/dominio.md` e lo stato nel `README.md`.
3. Elenca all'utente le ipotesi che hai dovuto fare.
