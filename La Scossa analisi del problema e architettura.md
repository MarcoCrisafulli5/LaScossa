# La Scossa: analisi del problema e architettura

Nov 24, 2026 · @Marco

Il gioco gira su un unico server C++ in un container Docker: il server possiede tutto lo stato della partita, lo fa evolvere con macchine a stati e invia a ogni schermo solo ciò che quello schermo può vedere. L'interfaccia è in React.

Le scelte principali:

- **Server autoritativo in C++20.** Plancia e pubblico sono client "stupidi": la plancia invia eventi, il pubblico disegna. Nessuna logica di gioco nel browser.
- **Minimo di librerie:** Crow per HTTP e WebSocket, nlohmann/json per il JSON; macchine a stati e lettore CSV scritti in casa.
- **Macchine a stati con enum e switch** per serata, modalità e domanda: stati ed eventi dichiarati in uno schema JSON, transizioni verificate nei test e visibili nel debug.
- **Schema JSON unico** da cui si generano le struct C++ e i tipi TypeScript del frontend.
- **Due proiezioni dello stato.** Il server calcola una vista "plancia" e una vista "pubblico"; le soluzioni non arrivano mai al browser del pubblico.
- **Registro eventi come fonte di verità.** Ripresa dopo un crash, annulla e debug derivano tutti da qui.
- **Timer sul server**, come eventi della macchina a stati.
- **Contenuti in CSV e parametri in un file di configurazione**, letti all'avvio e validati.

Base: il [documento dei requisiti](https://claude.ai/artifact/1tp7f5uQp7v87fpr1Kq6ji).

## Analisi del problema

Il problema è un sistema di regia in tempo reale: un operatore umano guida passo per passo un gioco con regole diverse per modalità, davanti a un pubblico, senza possibilità di fermarsi a correggere.

### Concetti del dominio

| Concetto | Cos'è | Note |
| --- | --- | --- |
| Serata | L'intera partita: concorrenti, modalità giocate, registro | Una sola attiva alla volta |
| Modalità | Una delle 8 (Estas, ESA, Date, La Scossa, Triello, Centos, Ghigliottina, Duelli) | Scelta liberamente dopo la precedente |
| Round | Un blocco di domande con contesto comune (categoria, 4 anni, materia) | Non tutte le modalità lo usano |
| Domanda | L'unità giocabile: testo, alternative o indizi, soluzione | Viene dai CSV |
| Concorrente | Nome ed eventuale foto&#32; | Numero da configurazione; i Duelli ne usano 2 |
| Turno | Quale concorrente deve rispondere ora - solo per duello con nome e foto. Nelle altre modalita' e' gestito dai conduttori e deve essere anonimo. | Passa per errore, per timer o a comando |
| Timer | Conto alla rovescia con etichetta, pausabile | Uno alla volta, due nei Duelli |
| Evento | Un fatto con nome: clic in plancia o timer scaduto | Unico modo per cambiare lo stato |
| Riquadro punteggio | Testo libero compilato in plancia | Mostrato o nascosto a comando |

### Caratteristiche che guidano l'architettura

1. **Un solo scrittore.** Solo la plancia cambia lo stato, quindi non ci sono conflitti da risolvere: basta una coda ordinata di eventi.
2. **Informazioni asimmetriche.** Plancia e pubblico vedono lo stesso stato con livelli di dettaglio diversi; il segreto va protetto nel server, non nascosto nel browser.
3. **Regole eterogenee, struttura comune.** Le 8 modalità differiscono nelle regole ma condividono turni, timer, selezione giallo → verde/rosso, rivelazione.
4. **Tempo reale ma a bassa frequenza.** Pochi eventi al minuto, latenza percepibile sopra circa 200 ms: un singolo processo basta e avanza.
5. **Errore umano frequente.** Ogni azione deve essere annullabile e ogni stato ricostruibile.
6. **Uso una volta l'anno.** Il sistema va riaperto dopo mesi da chi non lo ha scritto: semplicità e leggibilità contano più delle prestazioni.

## Vista d'insieme

Un container con un solo processo C++ contiene tutta la logica; plancia e pubblico sono due pagine web dello stesso server, aperte nel browser.

&#91;embedded content: componenti · 1 container, 2 browser\]

Il **motore a stati** è l'unico punto che cambia lo stato. Il **gateway** riceve gli eventi dalla plancia e rimanda a ogni browser la sua vista, calcolata dalle **proiezioni**: il pubblico non riceve mai soluzioni o domande future. Il **timer** e la **cartella dati** parlano solo con il motore.

## Stack tecnologico proposto

Deciso il 24/11: server in C++20 con il minimo di librerie, interfaccia in React + Vite. Prima il frontend, poi il confronto delle alternative per il server e lo stack scelto.

| Frontend | Scelta | Perché |
| --- | --- | --- |
| Interfaccia | React + Vite, in TypeScript | Componenti riusabili tra plancia e pubblico; animazioni in CSS per i cambi colore |
| Tipi dei messaggi | Generati dallo schema JSON | Nessun tipo scritto a mano, allineati al C++ |
| Librerie aggiuntive | Nessuna | WebSocket nativo del browser, niente gestori di stato esterni: lo stato arriva già pronto dal server |

Alternativa scartata: logica nel browser con le schede che si parlano direttamente. Più semplice oggi, ma non regge la plancia su un altro dispositivo e lascia le soluzioni nel browser del pubblico.

### Linguaggio del server: alternative valutate

Il frontend resta React + Vite in ogni caso; cambia solo il server. Le prestazioni non contano (pochi eventi al minuto): decidono manutenibilità, librerie per WebSocket e macchine a stati, e chi manterrà il codice.

| Opzione | Macchine a stati | HTTP + WebSocket | Pro | Contro |
| --- | --- | --- | --- | --- |
| TypeScript (Node.js) | XState v5, con ispettore visuale | Fastify + @fastify/websocket | Tipi di stato ed eventi condivisi con React; un solo linguaggio; avvio rapido | Errori di tipo solo in compilazione, runtime meno rigoroso |
| C++ (C++20) — scelto | Nessuna libreria: enum class + switch, con -Wswitch-enum -Werror | Crow (header-only) | Linguaggio che conosci; FSM molto esplicite e verificate dal compilatore; binario unico e piccolo nel container | Tipi da allineare con TypeScript (risolto con lo schema JSON e il generatore); più codice per HTTP, JSON e CSV; build Docker multi-architettura più lenta; ispettore delle FSM da fare in casa (pagina /debug) |
| Go | Libreria looplab/fsm o macchine scritte a mano | Libreria standard + gorilla/websocket | Semplice, binario unico, ottimo per server piccoli | Terzo linguaggio da imparare se non lo usi già; tipi da duplicare |
| Python | python-statemachine o transitions | FastAPI + WebSocket | Rapido da scrivere e leggere | Tipi da duplicare; controlli solo a runtime |

In C++ l'architettura resta la stessa: motore a stati, proiezioni, registro eventi e timer diventano classi C++, il protocollo JSON non cambia e il frontend React non se ne accorge. Il contratto tra server e frontend è uno schema JSON unico da cui si generano i tipi TypeScript e le strutture C++.

### Stack scelto: C++ con poche librerie

Con il server in C++ Node.js sparisce dall'esecuzione: serve solo durante la build per compilare React in file statici, che poi il server C++ distribuisce da solo. Nel container finale ci sono un binario e una cartella di file HTML, JS e CSS.

| Esigenza | Scelta minima | Note |
| --- | --- | --- |
| HTTP, file statici, WebSocket | Crow (header-only, usa Asio standalone) | Un solo processo per pagine e tempo reale |
| JSON | nlohmann/json (header-only) | Messaggi, configurazione, registro eventi |
| Macchine a stati | Nessuna libreria: FSM scritte a mano con enum + switch | Stati ed eventi sono dichiarati nello schema JSON (da cui si generano gli enum); la tabella delle transizioni serve a test e debug |
| CSV | Nessuna libreria: lettore interno | Formato semplice (punto e virgola, UTF-8, una riga per record) |
| Test | doctest (header-only) | Test delle FSM inviando sequenze di eventi |
| Build | CMake + Docker multi-stage | Stadio 1: Node compila React; stadio 2: compila il C++; immagine finale senza Node e senza compilatore |

Totale: due librerie per il server (Crow, nlohmann/json) più una per i test, tutte header-only.

**Schema JSON unico.** Un file `schema/` descrive eventi, viste di plancia e pubblico e tabelle di transizione. Un piccolo generatore produce da qui le struct C++ e i tipi TypeScript per React: cambi una volta, si aggiornano entrambi i lati e la compilazione segnala le differenze.

## Motore a stati

Tre livelli annidati: la macchina della **serata** contiene quella della **modalità** attiva, che contiene quella della **domanda** corrente. Un evento arriva alla serata e scende al livello che sa gestirlo.

1. **Serata:** `Lobby` → `SceltaModalità` → `InGioco(modalità)` → di nuovo `SceltaModalità` → `Fine`. Gestisce concorrenti, riquadro punteggio, blackout e pausa generale, validi in ogni stato.
2. **Modalità:** una macchina per modalità (es. Date: `RoundPronto` → `DomandaInGioco` → …, come nel documento dei requisiti). Decide cambio round e fine modalità.
3. **Domanda:** il ciclo comune `Mostrata` → `Selezionata` (giallo) → `Giusta` (verde) / `Sbagliata` (rosso) → turno successivo. Le modalità a risposta aperta (ESA, Centos, Duelli) usano una variante senza selezione.

In C++ ogni macchina è codice semplice e leggibile: gli stati sono un `enum class`, ogni stato ha una funzione gestore (statica o libera, non puntatore a funzione membro) che con uno `switch` sugli eventi decide guardia, azione e prossimo stato. Il compilatore gira con `-Wswitch-enum -Werror`, così un evento o uno stato dimenticato blocca la build; i casi lunghi diventano funzioni a parte. L'annidamento è composizione: la serata contiene la macchina della modalità attiva, che contiene quella della domanda.

La tabella delle transizioni nello schema JSON non guida l'esecuzione: serve a **formalizzare** stati ed eventi (da cui si generano gli `enum`), a **verificare** nei test che ogni transizione eseguita sia prevista, e a disegnare i diagrammi nella pagina di debug.

### Principi di progettazione delle FSM

Pratiche consolidate nel mondo embedded, applicate al motore (non ricavate dalla discussione Reddit segnalata, che non è stato possibile aprire):

- **Run-to-completion:** un evento alla volta da una coda; nessun evento interrompe una transizione in corso.
- **Azioni di ingresso e uscita** per stato (es. entrando in `Selezionata` si colora la risposta, uscendo si ferma il timer), invece di ripetere il codice su ogni transizione.
- **Stati gerarchici:** un evento non gestito dallo stato corrente risale al genitore (es. `PAUSA_GENERALE` gestito una volta sola dalla serata), come negli statechart di Harel e nel framework QP di Samek.
- **Switch in esecuzione, tabella per i controlli:** lo switch resta il codice più facile da leggere per chi arriva dopo; la tabella nello schema permette controlli automatici (ogni stato raggiungibile, nessun evento ambiguo) e il debug.
- **Gestori brevi e senza attese:** nessuna pausa o lettura bloccante dentro una transizione; il tempo entra solo come evento del timer.
- **Ogni transizione tracciata:** stato di partenza, evento, guardia, stato di arrivo nel registro.
- **Evento non previsto = errore esplicito**, mai ignorato in silenzio.

### Regole del motore

- **Eventi tipizzati**, in maiuscolo: `AVVIA_ROUND`, `SELEZIONA {opzione}`, `CONFERMA_GIUSTA`, `CONFERMA_SBAGLIATA`, `PASSA_TURNO`, `RIVELA`, `TIMER_SCADUTO {timerId}`, `PAUSA_TIMER {timerId}`, `PAUSA_GENERALE`, `ANNULLA`.
- **Guardie** sulle transizioni: un evento non ammesso nello stato corrente viene rifiutato e la plancia mostra il motivo.
- **Riuso:** la macchina della domanda e i servizi timer/turni sono comuni; ogni modalità aggiunge solo le sue regole.
- **Nessun effetto nascosto:** la macchina non legge l'orologio né il disco. Tempo e casualità (lettere dei Duelli) entrano come dati dell'evento, così ogni partita si riproduce identica.

### Timer

Un servizio timer nel server, fuori dalla macchina a stati. Ogni timer ha id, etichetta, durata e stato (`fermo`, `in corso`, `in pausa`). Nello stato si salva l'istante di scadenza e il tempo residuo; i browser disegnano il conto alla rovescia localmente. Alla scadenza il servizio invia `TIMER_SCADUTO` al motore come qualsiasi altro evento.

I timer sono di due tipi (deciso il 27/11, RC-05). I **timer di gioco** sono quelli appena descritti: vivono nel server, la loro scadenza è un evento registrato e si fermano con la Pausa generale. I **timer liberi** li crea la plancia per momenti che la regia non controlla: passano dal server solo per arrivare al pubblico, nessuna macchina li legge e non generano eventi di gioco. Da decidere se sopravvivono a un crash e se la Pausa generale li ferma.

## Duelli

Il duello è una seconda partita nello stesso server, accanto alla serata (deciso il 27/11, requisiti RC-15 … RC-20). La Regia ne gestisce due, ognuna con la sua macchina, il suo registro e il suo Annulla. Mentre il duello è in corso la serata resta in pausa.

### Approcci valutati

| Punto | Scelta | Alternative scartate |
| --- | --- | --- |
| Dove vive il duello | Due partite indipendenti nella Regia, serata e duelli, ognuna con il suo `.jsonl`. La `Serata` e il suo motore restano come sono | Modalità 8 che sospende la serata: un solo registro, ma l'Annulla di una plancia toglierebbe i clic dell'altra e lo storico vivrebbe in una sola sera. Seconda macchina dentro la stessa partita: Annulla per contesto da riscrivere, `Partita` più complessa |
| Proiettore | Un solo `/pubblico`; un comando "in onda" sceglie se mandare la vista della serata o quella del duello | Indirizzo separato a schermo intero: cambiare finestra in diretta rischia di proiettare la plancia |
| Storico delle parole | File `dati/duelli/usate.csv`, letto all'avvio e fissato nel registro all'apertura del duello, aggiornato quando una parola va in onda | Ricavato dal registro della serata: vale una sola sera. Ricavato da tutti i registri: cancellare `partite/` azzera lo storico senza volerlo |
| Lettere | Automatiche ogni N secondi, inserite dal servizio timer come eventi; l'ultima solo a conferma della plancia | Solo a comando: troppo lavoro per il conduttore in un duello veloce |

Con la serata in pausa anche la modalità 8 funzionerebbe; le due partite restano preferite per l'Annulla separato e lo storico tra serate.

### Come funziona

- **Avvio:** dalla plancia del duello, con i due concorrenti scelti da `concorrenti.csv` (nome e foto). La Regia mette in pausa la serata e apre il duello in `partite/<data>-duelli.jsonl`. A duello finito la serata riprende dal punto esatto.
- **In onda:** è un evento della serata, come il blackout, così sopravvive a un crash. Il pubblico riceve una vista che è o la serata o il duello.
- **Protocollo:** un ruolo nuovo `plancia-duello`; gli eventi dei due giochi passano dallo stesso mutex, uno alla volta.
- **Parole:** la Regia sceglie a caso tra quelle non ancora usate e scrive la scelta nell'evento, così il replay è identico. Quando la parola va in onda la aggiunge allo storico, dopo aver scritto il registro; resta bruciata anche se l'evento si annulla.
- **Vista del pubblico:** nomi e foto, definizione, caselle con le sole lettere scoperte, due orologi. Nessun campo per la parola intera.

### Orologi a scacchi e lettere

- **Stato:** residuo di sinistra, residuo di destra, chi ha la palla, istante in cui è partito il suo orologio (o fermo). Il tempo entra solo da `Evento::t_ms`.
- **Passaggio di palla:** a ogni risposta, giusta o sbagliata, all'istante t, chi aveva la palla perde t − avvio dal residuo, la palla passa all'altro e il suo orologio parte da t.
- **Servizio timer:** dopo ogni evento ricalcola dallo stato le prossime scadenze e inserisce `TIMER_SCADUTO {lato}` e `RIVELA_LETTERA` (ogni N secondi di orologio che corre, posizione casuale scritta nell'evento). Prima dell'ultima lettera si ferma e aspetta la conferma della plancia.
- **Corse:** se un clic e una scadenza arrivano insieme vince il primo entrato nel mutex; il secondo è rifiutato.
- **Annulla:** ogni evento porta la sua origine (conduttore o automatico). Annulla toglie l'ultimo evento del conduttore e gli eventi automatici venuti dopo; gli orologi tornano al valore di quel momento e restano in pausa. Dopo un crash si riprende allo stesso modo, in pausa.

## Flusso di un evento

Un clic in plancia diventa un evento; il server lo controlla, lo scrive nel registro e solo dopo aggiorna gli schermi.

&#91;embedded content: flusso di un evento · 6 passi, 1 controllo\]

Gli eventi sono elaborati uno alla volta, in ordine di arrivo. Il registro si scrive prima di inviare le viste: se il server cade a metà, al riavvio lo stato è quello che i conduttori hanno visto. Obiettivo dal documento dei requisiti: aggiornamento sul proiettore entro 0,5 s.

## Protocollo tra client e server

Un solo canale WebSocket per client, messaggi JSON. Il client dichiara il ruolo alla connessione; il server risponde sempre con lo stato completo della sua vista, poi con gli aggiornamenti.

| Messaggio | Direzione | Contenuto |
| --- | --- | --- |
| `hello` | client → server | ruolo (`plancia` o `pubblico`), PIN se plancia (opzionale) |
| `snapshot` | server → client | vista completa per quel ruolo, numero di sequenza, ora del server |
| `event` | plancia → server | evento con nome e dati, id univoco per evitare doppi invii |
| `ack` / `reject` | server → plancia | evento accettato, oppure rifiutato con motivo (es. "non ammesso in Selezionata") |
| `update` | server → client | nuova vista dopo ogni evento accettato, con numero di sequenza |
| `ping` / `pong` | entrambi | controllo della connessione ogni 5 s; la plancia mostra "disconnesso" se salta |

Per semplicità `update` invia la vista intera, non le differenze: è piccola (pochi KB) e un client che perde un messaggio si riallinea da solo al successivo. Alla riconnessione il client riceve di nuovo lo `snapshot`.

## Dati: configurazione e contenuti

Tutti i dati della serata stanno in una cartella `dati/` montata nel container: si cambiano domande, foto e parametri senza ricostruire l'immagine.

```
dati/
  config.json          parametri di gioco
  concorrenti.csv
  contenuti/
    mod1_estas.csv  …  mod8_duelli.csv
  foto/                foto dei concorrenti per i Duelli
  grafica/             logo, sfondi, suoni (segnaposto per ora)
  partite/             registri eventi, creati dal server
```

Esempio di `config.json` (valori indicativi):

```json
{
  "concorrenti": { "numero": 6 },
  "plancia": { "pin": null },
  "estas": { "attesaEsitoMs": 2000 },
  "esa": { "tempoLetturaIndizioSec": 3, "timerRispostaSec": 30 },
  "centos": { "turnoSec": 5 },
  "ghigliottina": { "timerSec": 120 },
  "duelli": { "timerSec": 60, "lettereOgniSec": 5 },
  "selezione": { "escluse": ["esa", "centos", "duelli"] }
}
```

All'avvio il server legge e valida tutto con un validatore interno basato sullo schema. Un errore (colonna mancante, risposta corretta non tra le alternative) blocca l'avvio con un messaggio del tipo "mod4\_lascossa.csv riga 12: sbagliata deve essere tra 1 e 9". La plancia ha anche un pulsante "Ricarica contenuti" usabile tra una modalità e l'altra.

## Persistenza, annulla, recupero e debug

La fonte di verità è il registro degli eventi, non lo stato: lo stato è sempre ricostruibile rigiocando gli eventi dall'inizio.

- **Registro:** ogni evento accettato è aggiunto a `partite/<data>.jsonl`, una riga per evento con numero di sequenza, ora, evento e stato di partenza (quello di arrivo si ricava rigiocando). Scrittura immediata su disco.
- **Istantanee:** non previste nella prima versione (il replay completo è abbastanza veloce); si aggiungono solo se le misure lo richiedono.
- **Ripresa dopo un crash:** al riavvio il server rigioca il registro e torna esattamente dov'era. I browser si riconnettono da soli.
- **Annulla:** il server rigioca il registro senza l'ultimo evento. Funziona anche per più passi indietro. Gli eventi annullati restano nel registro marcati come tali.
- **Debug:** una pagina `/debug` (solo plancia) mostra stato corrente, ultimi eventi e transizioni rifiutate. Un registro si può rigiocare evento per evento in un test; la pagina /debug disegna anche il diagramma di ogni FSM a partire dalle tabelle dello schema.
- **Test:** per ogni modalità, test automatici che inviano sequenze di eventi alla macchina e controllano lo stato risultante, senza browser.

## Struttura del progetto

Un unico repository: lo schema JSON al centro, il server C++ e il frontend React generano da lì i propri tipi. La logica di gioco è una libreria C++ separata dal server, così si testa da sola.

```
lascossa/
  schema/        eventi, viste, tabelle di transizione (JSON)
  tools/         generatore C++ eseguito da CMake: schema -> header C++ e tipi TS
  core/          libreria C++: motore FSM, modalità, proiezioni, registro
    modalita/    una cartella per modalità (estas, esa, date, …)
    test/        test doctest delle FSM
  server/        eseguibile C++: Crow, WebSocket, timer, caricamento dati
  web/           React + Vite: /plancia, /pubblico, /debug
  third_party/   Crow, nlohmann/json, doctest (header-only)
  dati/          esempio di configurazione e CSV
  CMakeLists.txt
  Dockerfile     stadio Node (build web) + stadio C++ + immagine finale minima
  docker-compose.yml
```

Avvio previsto in sala: `docker compose up`, poi aprire `http://localhost:8080/plancia` sul portatile e `http://localhost:8080/pubblico` a schermo intero sul proiettore. In alternativa senza Docker, dopo la build: `./lascossa-server --dati ./dati`.

## Rischi tecnici e decisioni aperte

| Rischio o decisione | Impatto | Proposta |
| --- | --- | --- |
| Motore FSM scritto in casa | Più codice da scrivere e testare rispetto a una libreria | Switch semplici con -Wswitch-enum -Werror; test che confrontano ogni transizione eseguita con la tabella dello schema |
| Generatore di tipi dallo schema | Va scritto e mantenuto | Deciso: generatore in C++ (usa nlohmann/json, nessuna dipendenza in più) nella cartella tools/, compilato ed eseguito da CMake con add\_custom\_command prima del server; produce gli header C++ e il file dei tipi TypeScript per React |
| Server HTTP e WebSocket in C++ | Meno documentazione ed esempi rispetto a Node | Crow copre pagine statiche e WebSocket con poche righe. Se la connessione cade, il client la ricrea da solo e riceve di nuovo lo snapshot |
| Build multi-architettura del C++ | Build Docker lenta in emulazione | Build sul Mac (arm64) per l'uso in sala; amd64 solo se serve |
| Annulla rigiocando tutto il registro | Basso. "Lento" = annulla che impiega più di circa 100 ms, soglia oltre cui il conduttore percepisce un ritardo. In C++ rigiocare qualche migliaio di eventi (lettura JSON + switch) richiede nell'ordine dei millisecondi (stima, da misurare) | Nessuna istantanea nella prima versione; un test misura il tempo di replay su un registro di 10.000 eventi e le istantanee si aggiungono solo se supera 100 ms |
| Orologi di server e browser non allineati | Conto alla rovescia sfasato sul proiettore | Il server invia la sua ora in ogni messaggio; il client controlla lo scarto con il proprio orologio e corregge solo se supera una soglia (parametro in config, es. 250 ms); altrimenti non fa nulla |
| Docker Desktop sul Mac non avviato o aggiornato la sera stessa | Il gioco non parte | Binario avviabile anche senza Docker; checklist pre-serata |
| Schermo pubblico 16:9 con risoluzione ignota | Testi tagliati | Layout in unità relative al viewport; prova sul proiettore alla Fase 1 |
| Decisione aperta: PIN sulla plancia | Serve solo se la plancia è raggiungibile in Wi-Fi | Predisposto in configurazione, disattivato di default |
