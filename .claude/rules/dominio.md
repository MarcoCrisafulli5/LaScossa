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

## Modalità implementate

- **1. Estas o non Estas**: categoria → oggetti; opzione 0 = Estas, 1 = Non Estas. Cambio categoria
  solo a oggetto concluso; una categoria ripresa continua dall'oggetto successivo.
- Le altre 7 sono nell'enum `IdModalita` ma `Serata::disponibile()` restituisce false.

## Ipotesi da confermare con l'utente (non trasformarle in certezze)

- Estas: dopo una risposta sbagliata il turno passa al concorrente successivo; dopo una giusta resta.
- Timer (RC-05): due tipi (effimeri solo frontend; legati a domande/eventi). Analisi del problema
  ancora da fare: non implementare timer senza chiedere.
- Le domande aperte di ogni modalità sono nella colonna "Da chiarire" della sezione 5 dei requisiti.

## Requisiti da formalizzare (sezione 8.1 dei requisiti)

- **Duelli (24/11)**: il duello può avvenire in qualsiasi momento della serata, su una schermata
  pubblico separata con una plancia dedicata, e deve tenere traccia delle parole già uscite per non
  riproporle. Contraddice il modello attuale ("una modalità attiva alla volta" dentro la Serata):
  **non implementare i Duelli come le altre modalità** finché il problema non è analizzato con l'utente.

## Decisioni già prese (24/11/2026)

- Server C++20 in Docker (opzione B), raggiunto in localhost dallo stesso MacBook M4; immagine multi-arch.
- Minimo di librerie: Crow, nlohmann/json, doctest. FSM e lettore CSV scritti in casa.
- React + Vite + TypeScript, nessuna libreria aggiuntiva (niente router, niente state manager).
- Niente punteggio automatico: riquadro a comparsa compilato in plancia.
- Ordine delle modalità libero; nessuna scaletta fissa.
- Grafica segnaposto, sostituibile senza toccare il codice.
- PIN della plancia predisposto in configurazione, disattivato di default.
