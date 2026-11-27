# Dominio del gioco

## Glossario

| Termine | Significato |
| --- | --- |
| Serata | L'intera partita: modalità giocate, registro. Stati: Lobby → SceltaModalita ⇄ InGioco → Chiusa |
| Modalità | Una delle 8 (Estas, ESA, Date, La Scossa, Triello, Centos, Ghigliottina, Duelli), scelta liberamente dopo la precedente |
| Round | Blocco di domande con contesto comune (categoria, 4 anni, materia) |
| Domanda | Unità giocabile: testo, opzioni o indizi, soluzione. Ciclo: Mostrata → Selezionata → Giusta / Sbagliata, oppure Rivelata |
| Concorrente | Anonimo, tranne nei Duelli (nome e foto). Numero da `config.json`; ogni modalità è agnostica al numero |
| Turno | Chi risponde ora. Gestito dai conduttori, mostrato solo in plancia come numero |
| Plancia | Schermata dei conduttori: vede tutto, comanda |
| Pubblico | Schermata del proiettore: vede solo ciò che la sala deve vedere |
| Riquadro punteggio | Testo libero scritto in plancia, mostrato o nascosto sul pubblico. Il punteggio lo tengono i conduttori |
| Blackout | Pubblico nero con il logo, in ogni momento |
| Pausa generale | Congela i timer e blocca gli eventi di gioco |

## Regola giallo → verde / rosso (RC-06)

Nelle modalità a scelta il conduttore seleziona la risposta del concorrente (giallo sul pubblico),
poi conferma: la macchina confronta con la soluzione e la colora verde o rosso. Le modalità a
risposta aperta (ESA, Centos, Duelli) usano `CONFERMA_GIUSTA` / `CONFERMA_SBAGLIATA`: giudica il conduttore.
Colori in `web/src/stile.css` (variabili CSS), non nel codice dei componenti.

**Suspense (27/11)**: dopo la conferma il pubblico tiene il giallo per `<modalità>.attesaEsitoMs`
(config, default 2000) e poi colora; la plancia spegne "prossimo" e il cambio categoria per lo stesso
tempo. È solo frontend (`web/src/attesaEsito.ts`): il server registra l'esito subito e manda la
durata nella vista. Ogni modalità a scelta nuova ha il suo parametro e usa lo stesso hook.

## Modalità implementate

- **1. Estas o non Estas**: categoria → oggetti; opzione 0 = Estas, 1 = Non Estas. Prima di ogni
  oggetto c'è `AttesaOggetto` (25/11): il pubblico vede solo la categoria, la plancia vede l'oggetto in
  arrivo, `MOSTRA_DOMANDA` lo rivela. Cambio categoria solo a oggetto concluso o in attesa; una
  categoria ripresa continua dall'oggetto successivo.
- `MOSTRA_DOMANDA` è generico: usalo anche nelle altre modalità quando serve una pausa tra il
  contesto (round, materia, date) e la domanda.
- Le altre 7 sono nell'enum `IdModalita` ma `Serata::disponibile()` restituisce false.

## Ipotesi da confermare con l'utente (non trasformarle in certezze)

- Estas: dopo una risposta sbagliata il turno passa al concorrente successivo; dopo una giusta resta.
- Le domande aperte di ogni modalità sono nella colonna "Da chiarire" della sezione 5 dei requisiti.

## Timer (RC-05, deciso il 27/11)

- **Timer di gioco**: nel server (servizio della Regia), la scadenza è un evento registrato con
  `t_ms`; i browser disegnano solo il conto alla rovescia. Si fermano con la Pausa generale.
- **Timer liberi**: creati dalla plancia, passano dal server solo per arrivare al pubblico, nessuna
  macchina li legge. Aperto: se sopravvivono a un crash e se la Pausa generale li ferma.

## Duelli (RC-15 … RC-20, analizzati il 27/11; sezione 5.1 dei requisiti, sezione "Duelli" dell'architettura)

- **Non è la modalità 8 dentro la Serata**: è una seconda partita nella Regia, con registro
  (`partite/<data>-duelli.jsonl`), Annulla e plancia propri. Durante il duello la serata è in pausa.
- Un solo `/pubblico`: un comando "in onda" (evento della serata) sceglie la vista serata o duello.
- Storico parole in `dati/duelli/usate.csv`, valido tra serate; parola andata in onda = bruciata
  anche se annullata. La parola la sceglie a caso la Regia e la scrive nell'evento.
- Due orologi a scacchi: a ogni risposta, giusta o sbagliata, si ferma l'orologio di chi ha
  risposto e parte quello dell'altro. Lettere automatiche ogni N secondi (eventi del servizio timer con origine
  "automatico"), l'ultima solo a conferma. Annulla toglie l'ultimo evento del conduttore e gli
  automatici successivi, e lascia gli orologi in pausa.
- Le regole di gioco del duello sono rimandate (27/11): i punti "Da chiarire" della sezione 5.1
  restano aperti. Si può predisporre l'architettura (Regia con due partite, ruolo `plancia-duello`,
  "in onda", origine degli eventi), non la macchina a stati del duello.

## Decisioni già prese (24/11/2026)

- Server C++20 in Docker (opzione B), raggiunto in localhost dallo stesso MacBook M4; immagine multi-arch.
- Minimo di librerie: Crow, nlohmann/json, doctest. FSM e lettore CSV scritti in casa.
- React + Vite + TypeScript, nessuna libreria aggiuntiva (niente router, niente state manager).
- Niente punteggio automatico: riquadro a comparsa compilato in plancia.
- Ordine delle modalità libero; nessuna scaletta fissa.
- Grafica segnaposto, sostituibile senza toccare il codice.
- PIN della plancia predisposto in configurazione, disattivato di default.
