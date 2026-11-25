# Regole architetturali

Riassunto vincolante di "La Scossa analisi del problema e architettura.md". In caso di dubbio vale il documento.

## Server autoritativo, client stupidi

- Tutto lo stato della partita vive nel server C++. Plancia e pubblico non hanno logica di gioco.
- La plancia invia **eventi con nome** (`SELEZIONA`, `CONFERMA`, …) e disegna la vista che riceve.
  Non decide mai se un evento è valido: abilita un pulsante solo se l'evento è in `vista.ammessi`.
- Il pubblico disegna la propria vista e non invia eventi.

## Le soluzioni non arrivano mai al pubblico

- Il server calcola due proiezioni distinte (`core/include/lascossa/viste.hpp`): `VistaPlancia` e `VistaPubblico`.
- Le struct del pubblico **non hanno campi** per soluzioni, risposte corrette o domande future.
  Non aggiungerli mai, nemmeno "nascosti" o per comodità: la garanzia è nel tipo.
- La soluzione compare sul pubblico solo come stato di un'opzione già giudicata (`giusta`) o
  rivelata dal conduttore (`soluzione`).
- Ogni modalità nuova ha un test che verifica che il JSON del pubblico non contenga la soluzione.

## Macchine a stati

- Tre livelli annidati per composizione: **serata** → **modalità** attiva → **domanda** corrente.
- Stati in `enum class`, un gestore per stato con `switch` sugli eventi che elenca **tutti** i casi,
  senza `default`. La build usa `-Wswitch-enum -Werror`: un evento dimenticato blocca la compilazione.
- Run-to-completion: un evento alla volta (mutex nel server), nessuna attesa o I/O dentro una transizione.
- Evento non ammesso = `Esito::rifiuto("motivo leggibile")`, mai ignorato in silenzio.
- Ogni transizione va nella `Traccia` (macchina, da, evento, a).
- Le macchine non leggono orologio né disco: l'ora arriva in `Evento::t_ms`, il caso come dato dell'evento.
  Così ogni partita si riproduce identica rigiocando il registro.
- `Modalita::clona()` deve fare una copia profonda: serve a calcolare gli eventi ammessi su una copia.

## Registro eventi = fonte di verità

- Ogni evento accettato è scritto in `dati/partite/<data>.jsonl` e svuotato su disco **prima** di
  inviare le viste.
- Annulla = rigiocare il registro senza l'ultimo evento. Ripresa dopo crash = rigiocare il file.
- `ANNULLA` si registra come gli altri eventi: rigiocando le righe in ordine si ottiene lo stesso stato.
- Niente istantanee finché il replay di 10.000 eventi resta sotto 100 ms (oggi ~1 ms).

## Protocollo (WebSocket `/ws`, JSON)

- client→server: `hello {ruolo, pin?}`, `event {id, evento, arg?, testo?}`, `ping`.
- server→client: `snapshot`/`update {seq, oraServer, vista}`, `ack {id}`, `reject {id, motivo}`, `pong`.
- `update` porta la vista intera (pochi KB), non differenze. `id` unico per evento: un doppio invio
  riceve `ack` senza essere riapplicato.
- `NUOVA_PARTITA` è un comando del server (fuori dalle FSM), ammesso solo in Lobby o a serata chiusa.
- Il codice di rete (`server/src/main.cpp`) non contiene logica: consegna messaggi alla `Regia`,
  che si testa senza socket.

## Dati separati dal codice

- Contenuti in CSV (`dati/contenuti/`), parametri in `dati/config.json`: si cambiano senza ricompilare.
- Ogni parametro di gioco nuovo va in `config.json` con un default, non scritto nel codice.
