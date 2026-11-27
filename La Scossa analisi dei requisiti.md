# La Scossa: analisi dei requisiti

Nov 23, 2026 · @Marco

## 1. Obiettivo e contesto

L'app sostituisce il PowerPoint usato finora per condurre La Scossa, quiz parodia de L'Eredità, con un'applicazione web che gira in due schede del browser: la **plancia di comando** per i conduttori e lo **schermo pubblico** proiettato in sala.

Il PowerPoint ha tre limiti che l'app deve superare: lo stato di gioco (chi ha risposto, quali risposte sono già bruciate, i punti) vive solo nella testa dei conduttori; ciò che vede il pubblico e ciò che vedono i conduttori coincidono, quindi le soluzioni non possono stare a schermo; timer e animazioni sono rigidi e legati alla sequenza delle slide.

Fonte delle regole: il file *MODALITA' 1 - Estas o non Estas.md*, che descrive 8 modalità (7 giochi più i Duelli). Dove le regole non bastano, il documento segna un'ipotesi o una domanda aperta.

## 2. Attori e scenario d'uso

| Attore | Cosa vede | Cosa fa |
| --- | --- | --- |
| Conduttori (ipotesi: 2) | Plancia: domanda, soluzione, stato, pulsanti | Avviano round, segnano risposte giuste o sbagliate, gestiscono timer e turni |
| Pubblico in sala | Schermo pubblico sul proiettore | Guarda, non interagisce |
| Concorrenti | Lo stesso schermo pubblico | Rispondono a voce; nessun dispositivo in mano&#32; |
| Autore dei contenuti | Editor delle domande (prima della serata) | Prepara categorie, domande, date, parole, foto |

Scenario di riferimento (confermato il 24/11: MacBook M4, Wi-Fi presente in sala): un solo portatile collegato al proiettore come schermo esteso. La scheda pubblico va a schermo intero sul proiettore, la plancia resta sul monitor del portatile. I conduttori usano mouse, tastiera o touchpad del portatile.

Plancia sullo stesso dispositivo.

## 3. Architettura proposta

Un'unica applicazione web aperta in due schede dello stesso browser: la plancia tiene lo stato della partita e lo invia allo schermo pubblico, che si limita a mostrarlo.

&#91;embedded content: architettura proposta · 2 schede, 1 computer\]

Niente server e niente internet: le schede dello stesso browser si parlano direttamente. Lo stato salvato in locale permette di riaprire la plancia senza perdere la partita. Se si sceglie la plancia su un altro dispositivo, serve un piccolo server locale nel portatile e una rete Wi-Fi.

### 3.1 Opzioni: solo browser o server containerizzato

Decisione del 24/11: **server containerizzato da subito (opzione B)**. Copre sia il caso "un solo portatile" sia la plancia su tablet, e il costo in più cade tutto sulla base comune, non sulle 8 modalità.

|  | A. Solo browser, plancia predisposta | B. Server containerizzato |
| --- | --- | --- |
| Come funziona | Un file HTML aperto in due schede; le schede si parlano dentro il browser. Il canale di comunicazione è un modulo separato, sostituibile in futuro con un server | Un container (Docker) con un piccolo server web; da qualsiasi browser si apre l'indirizzo e si sceglie "Plancia" o "Pubblico". Plancia e pubblico si aggiornano in tempo reale (WebSocket) |
| Plancia su altro dispositivo | No, va rifatto il canale quando servirà | Sì, qualsiasi dispositivo sulla stessa rete Wi-Fi |
| Stesso portatile, senza rete | Sì | Sì: il browser si collega al container sullo stesso portatile (localhost), niente Wi-Fi né internet |
| Cosa serve in sala | Solo un browser | Docker installato sul portatile; per altri dispositivi, una rete Wi-Fi locale (anche un router portatile o l'hotspot del telefono) |
| Stato della partita | Nel browser della plancia | Nel server: si può ricaricare o chiudere qualsiasi scheda senza perdere nulla; più plance possono collegarsi insieme |
| Contenuti | File locali caricati dalla plancia | File nel container o caricati via plancia; foto servite dal server |
| Complessità di realizzazione | Bassa: tutto in un'unica pagina | Media: un server, la riconnessione automatica se la rete cade, il container. Stima: la base comune costa circa un terzo in più; le modalità costano uguale |
| Rischi | Rifare la comunicazione in futuro; nessuna resilienza se la scheda plancia si chiude | Docker non presente o bloccato sul portatile della serata; Wi-Fi della sala instabile; firewall che blocca la porta |

Mitigazioni per B: lo stesso server si avvia anche senza Docker (un comando o un eseguibile), così il container è comodo ma non obbligatorio; la plancia mostra sempre se è collegata; prova tecnica in sala già alla Fase 1. Per ora plancia e pubblico girano sullo stesso MacBook M4 (localhost); l'immagine Docker si pubblica multi-architettura (linux/arm64 e linux/amd64), così gira nativa sia sul Mac sia su un PC.

Il ruolo (plancia o pubblico) si sceglie all'apertura. Domanda collegata: la plancia va protetta con un PIN, per evitare che qualcuno in sala sulla stessa Wi-Fi la apra?

### 3.2 Ogni modalità è una macchina a stati

Ogni modalità, e dentro di essa ogni domanda, è modellata come una macchina a stati finiti (FSM): stati espliciti, eventi con nome, transizioni ammesse dichiarate in anticipo.

- **Eventi, non comandi liberi.** Ogni pulsante della plancia e ogni scadenza di timer è un evento (es. `SELEZIONA`, `CONFERMA_GIUSTA`, `TIMER_SCADUTO`). Il server applica solo le transizioni previste dallo stato corrente e rifiuta le altre.
- **Stato unico e visibile.** La plancia mostra sempre lo stato corrente e solo i pulsanti degli eventi ammessi; lo schermo pubblico disegna lo stato, senza logica propria.
- **Registro delle transizioni.** Ogni transizione è salvata con ora, evento, stato di partenza e di arrivo: serve per il debug e per ripercorrere una partita.
- **Annulla = torna allo stato precedente**, letto dal registro.
- **Pausa** congela i timer senza cambiare stato.
- **Due livelli:** una FSM della serata (scelta della modalità e del round) che contiene le FSM delle singole modalità.

&#91;embedded content: esempio: Modalità 3 Date come macchina a stati\]

Le FSM delle altre modalità si disegnano allo stesso modo nella fase di progettazione, una per modalità.

Valutare containerizzazione per semplicita' di esecuzione e gestione. Docker.

In tal caso definire archiettura server/client . da valutare.

## 4. Requisiti funzionali comuni

Questi requisiti valgono per tutte le modalità e vanno costruiti una volta sola.

| ID | Requisito | Priorità |
| --- | --- | --- |
| RC-01 | Aprire la plancia e lo schermo pubblico come due schede dello stesso browser; ogni azione in plancia si riflette sul pubblico in meno di 0,5 s | Must |
| RC-02 | La plancia mostra informazioni riservate (soluzioni, domanda successiva, note) che non compaiono mai sullo schermo pubblico | Must |
| RC-03 | Menu della serata: scegliere la modalità, il round e il set di domande da giocare, in qualsiasi ordine; al termine di ogni modalità si sceglie liberamente la successiva, nessuna scaletta fissa | Must |
| RC-04 | Anagrafica concorrenti: nomi (e foto per i Duelli), concorrente di turno evidenziato, passaggio al successivo. Numero di concorrenti nel file di configurazione; tutte le modalità funzionano con qualsiasi numero, tranne i Duelli (sempre 2, con foto) | Must |
| RC-05 | Timer comandati dalla plancia: avvio, pausa, reset, durata configurabile, visibili su entrambe le schede; pulsante unico "Pausa generale" che ferma e riprende tutti i timer attivi insieme (anche i 2 dei Duelli e il passaggio automatico di Centos). Ogni timer ha un'etichetta in plancia (es. "ESA – risposta", "Duello – sx") e si mette in pausa anche da solo. Di norma è attivo un solo timer alla volta; unica eccezione i Duelli, con un timer per giocatore.\
I timer sono di 2 tipi. Effimeri solo frontend, non fanno nulla, mostrano il tempo e sono controllabili dalla plancia con start, stop, restart. Timer collegati a domande ed eventi in gioco che sono definiti a compile e modificabili a game time. Sui timer c'e' da analizzare bene.ANALISI DEL PROBLEMA NECESSARIA. | Must |
| RC-06 | Pulsanti di esito: risposta giusta, sbagliata, passa turno, mostra soluzione. Regola standard per tutte le modalità a scelta: il conduttore seleziona in plancia la risposta del concorrente, che sul pubblico si evidenzia in giallo; poi la conferma e diventa verde se giusta, rossa se sbagliata. Ogni modalità può disattivarla (ipotesi: ESA, Centos e Duelli, che hanno risposte aperte) | Must |
| RC-07 | Annulla ultima azione, per rimediare a un clic sbagliato in diretta | Must |
| RC-08 | Schermata "di attesa" con logo La Scossa, richiamabile in ogni momento (blackout del pubblico) | Should |
| RC-09 | Contenuti caricati da file (domande, categorie, date, parole, foto), modificabili senza toccare il codice | Must |
| RC-10 | Scorciatoie da tastiera per le azioni più frequenti. non necessario ma potrebbe tornare utile. | Should |
| RC-11 | Effetti sonori (giusto, sbagliato, scossa, timer scaduto) attivabili dalla plancia | Could |
| RC-12 | Punteggio o montepremi: lo tengono i conduttori. L'app offre un riquadro a comparsa, compilato in plancia, che si mostra e si nasconde sullo schermo pubblico a comando | Da definire |
| RC-13 | Segnare le domande già usate, per non riproporle nella stessa serata | MUST |
| RC-14 | Riprendere la partita dopo un ricaricamento accidentale della pagina | MUST |

### 4.1 Formato dei contenuti (proposta)

Un file CSV per modalità, una riga per domanda o oggetto, separatore punto e virgola (Excel in italiano lo apre e lo salva così), codifica UTF-8. Il recupero dei contenuti dal PowerPoint e i parser si fanno in seguito.

| File | Colonne |
| --- | --- |
| mod1\_estas.csv | categoria; oggetto; appartiene (si/no) |
| mod2\_esa.csv | indizio1; indizio2; indizio3; soluzione |
| mod3\_date.csv | round; anno1; anno2; anno3; anno4; domanda; anno\_corretto (una riga per domanda, stessi 4 anni ripetuti nelle righe dello stesso round) |
| mod4\_lascossa.csv | round; domanda; alt1 … alt9; sbagliata (1-9) |
| mod5\_triello.csv | materia; domanda; a; b; c; d; corretta (a-d); valore (punti o SCOSSA) |
| mod6\_centos.csv | domanda; risposta |
| mod7\_ghigliottina.csv | round; parola\_a; parola\_b; corretta (a/b); soluzione\_finale |
| mod8\_duelli.csv | parola; definizione |
| concorrenti.csv | nome; foto (nome file nella cartella foto) |

Esempio per la Modalità 1:

```csv
categoria;oggetto;appartiene
Piatti tipici regionali;Arancini;si
Piatti tipici regionali;Sushi;no
Piatti tipici regionali;Pizzoccheri;si
```

## 5. Requisiti per modalità

Per ogni modalità: cosa mostra lo schermo pubblico, cosa offre la plancia e cosa manca nelle regole.

| Modalità | Schermo pubblico | Plancia conduttori | Da chiarire |
| --- | --- | --- | --- |
| 1. Estas o non Estas | Categoria in alto, oggetto corrente; due pulsanti (Estas / Non Estas) che si illuminano sulla risposta data. Prima di ogni oggetto compare solo la categoria, grande, per soffermarsi su regole e dubbi senza la domanda a schermo (deciso il 25/11) | Elenco categorie (più di 10 oggetti ciascuna) con la soluzione accanto a ogni oggetto; cambio categoria a ogni domanda; segna giusto o sbagliato; mentre il pubblico vede solo la categoria, anteprima dell'oggetto in arrivo con la soluzione e pulsante "Mostra oggetto" | Risolto: segue la regola giallo → verde/rosso di RC-06. Quanti oggetti per concorrente? |
| 2. ESA | Indizi che compaiono in sequenza, poi timer; frase corretta rivelata | Frase soluzione sempre visibile; avvio sequenza indizi; pulsante "Rivela" in qualsiasi momento; giudizio del conduttore sulla frase esatta | Tempo di lettura tra un indizio e l'altro: 3 s (parametro). Durata del timer da definire; gli indizi sono sempre 3? |
| 3. Date | Domanda e 4 date; la data sbagliata diventa rossa e resta rossa | Data corretta evidenziata; clic su una data = risposta del concorrente di turno; errore passa al successivo; nuova domanda solo dopo risposta giusta; pulsante cambio round (cambia le date) | Risolto: date solo come anno (es. 1968); ogni round ha 4 date proprie e può avere più domande, in numero libero. |
| 4. La Scossa | Domanda in alto e 9 alternative | Alternativa sbagliata evidenziata; clic sull'alternativa scelta; cambio round (domanda + 9 alternative) | Risolto: l'alternativa scelta si evidenzia prima (colore di selezione: parametro grafico, definito nel frontend), poi diventa verde se corretta e rossa se è quella sbagliata (la scossa). Cosa succede al concorrente dopo la scossa: rinviato a una fase successiva. |
| 5. Triello | Materia del round, domanda, 4 alternative, valore (punti o scossa) | 6 round per materia; risposta corretta evidenziata; il round finisce alla risposta giusta | Gestione punti rinviata dalle regole; chi sceglie il valore? È a turni tra 3 concorrenti? |
| 6. Centos | Domanda a risposta aperta, timer a 5 s per turno, concorrente di turno | Risposta sempre visibile; timer avviato dai conduttori; passaggio automatico del turno ogni 5 s o a risposta sbagliata | Prima versione: si gioca solo il turno di risposta, senza vincitore né conteggio. Regole di vittoria ed eliminazione rinviate, con la struttura predisposta per aggiungerle. |
| 7. Ghigliottina | Coppie di parole (5 scelte), le parole scelte salgono in alto a destra; timer di 2 minuti con le 5 parole; poi la parola soluzione | Parola corretta di ogni coppia; registrazione della scelta; avvio timer; rivelazione | Se il concorrente sceglie la parola sbagliata cosa vede il pubblico (tipicamente il montepremi si dimezza)? |
| 8. Duelli | Due concorrenti con foto, sinistra e destra; parola o definizione con lettere che compaiono a caso; due timer | Soluzione; comparsa lettere automatica o a comando; ultima lettera solo a comando; alternanza sx/dx; due timer indipendenti | Le lettere compaiono a tempo o solo a clic? I timer sono a scalare come negli scacchi? |

## 6. Requisiti non funzionali

- **Funziona offline.** La sala potrebbe non avere internet: l'app parte da file locali, senza server né installazioni (ipotesi: basta aprire un file HTML in Chrome o Edge).
- **Leggibile da lontano.** Schermo pubblico in 16:9, testi grandi e ad alto contrasto, pensato per un proiettore anche poco luminoso.
- **Affidabile in diretta.** Nessun clic deve rompere la partita; ogni azione è annullabile; un ricaricamento non perde lo stato.
- **Semplice per i conduttori.** Un'azione = un pulsante grande, ben etichettato; nessuna finestra di conferma durante il gioco.
- **Contenuti separati dal codice.** Ogni anno si cambiano domande e foto senza programmatore, idealmente da un foglio di calcolo. Tutti i parametri di gioco (tempi di lettura, durate dei timer, numero di indizi, modalità escluse dalla regola giallo/verde/rosso) stanno in un unico file di configurazione, con valori predefiniti modificabili senza toccare il codice.
- **Identità grafica propria.** Grafica ispirata allo stile dei quiz televisivi ma con logo, colori e nome di La Scossa, senza riprodurre marchi o grafiche de L'Eredità.

## 7. Punti critici e rischi

Il punto più critico è la sincronizzazione tra le due schede; subito dopo vengono i punteggi, ancora non definiti.

| # | Punto critico | Perché conta | Proposta |
| --- | --- | --- | --- |
| 1 | Sincronizzazione plancia → pubblico | Se il pubblico resta indietro o si desincronizza, la diretta si blocca | Deciso: server in Docker sullo stesso portatile, raggiunto in localhost, con aggiornamenti in tempo reale e riconnessione automatica. In futuro la plancia potrà passare su un altro dispositivo senza riscrivere nulla |
| 2 | Punteggi e montepremi non definiti | Influenzano Triello, Ghigliottina, la scossa e il passaggio tra modalità | Deciso: niente punteggio automatico; i conduttori lo tengono e lo mostrano con il riquadro a comparsa (RC-12) |
| 3 | Timer precisi e coerenti | Centos (5 s), Ghigliottina (2 min), Duelli (2 timer): un timer che salta falsa il gioco | Il tempo lo calcola solo la plancia; il pubblico lo mostra e basta -&#32; |
| 4 | Errore del conduttore in diretta | Un clic sbagliato rivela la soluzione o brucia una risposta | Annulla ultima azione, separazione netta tra pulsanti "mostra" e "segna", tasto blackout |
| 5 | Fuga della soluzione sul proiettore | Se la plancia finisce sul proiettore (schermo duplicato invece che esteso) il gioco è rovinato | Avviso in plancia e checklist di setup prima della serata |
| 6 | Preparazione dei contenuti | Oggi i contenuti sono dentro il PowerPoint; ogni modalità ha un formato diverso | Un modello di foglio di calcolo per modalità e un'importazione; recupero dei contenuti degli anni passati |
| 7 | Regole incomplete | Mod. 2, 4, 5, 6, 7, 8 hanno punti aperti | Domande nella sezione 8; per ciascuna una soluzione di default modificabile |
| 8 | Hardware della sala | Risoluzione e luminosità del proiettore, audio, uscita video del portatile | Prova tecnica in sala con la prima versione |

## 8. Domande aperte

Le prime quattro decidono l'architettura; le altre si possono chiudere mentre si sviluppa.

1. La plancia sta sullo stesso portatile del proiettore o su un altro dispositivo (tablet, telefono)? Per il momento stesso dispositivo ma usiamo docker cosi' portatilita' garantita in futuro
2. Quanti concorrenti per serata e quanti per modalità? Ci sono eliminazioni tra una modalità e l'altra come nel programma originale? Questi sono parametri che vanno definiti nel file di conf, comunque le modalita' devono essere agnostiche al numero di concorrenti tranne il duello che appunto prevede 2 concorrenti con foto.
3. Esiste un montepremi o un punteggio che passa da una modalità all'altra? Cos'è esattamente una "scossa" nel regolamento? al momento no, lo terranno i conduttori. Possiamo realizzarlo come elemento popup che viene configurato lato plancia e mostrato su schermo e nascosto al bisogno.&#32;
4. Le modalità si giocano sempre nello stesso ordine o la scaletta cambia ogni anno? Dobbbiamo fare in modo che dopo ogni modalita' si possa scegliere la prossima, non vincoliamo l'ordine di gioco
5. Quanti conduttori e chi usa la plancia (uno solo o entrambi)?
6. Avete già logo, colori, font, suoni e sigla di La Scossa da riusare dal PowerPoint? Deciso il 24/11: per ora grafica segnaposto, sostituibile senza toccare il codice.
7. Possiamo avere il PowerPoint degli anni scorsi per recuperare contenuti e grafica?
8. Che computer, sistema operativo e browser usate in sala? C'è internet? macbook m4 probabilmente come computer, c'e' wifi
9. I punti aperti per modalità nella colonna "Da chiarire" della sezione 5.

### 8.1 Requisiti da formalizzare

Requisiti emersi durante lo sviluppo: vanno analizzati e trasformati in requisiti con un ID prima di implementarli.

| Annotato | Requisito | Stato |
| --- | --- | --- |
| 24/11 | **Duello in qualsiasi momento.** Il duello può avvenire in qualsiasi momento della serata, non solo come modalità scelta in scaletta. Si svolge in una schermata pubblico separata e ha una plancia di controllo dedicata. | Da analizzare |
| 24/11 | **Duelli senza ripetizioni.** Bisogna tenere traccia di parole e definizioni già uscite nei duelli, per non riproporle (estende RC-13 ai duelli). | Da analizzare |

Punti da analizzare: se il duello sospende la modalità in corso o gira in parallelo; cosa mostra intanto lo schermo pubblico principale; come si aprono la schermata e la plancia del duello; come funzionano registro e annulla con due giochi aperti; se le parole usate valgono per la serata o anche tra serate diverse.

## 9. Fasi di rilascio proposte

Si parte dalla base comune e da una modalità sola, per verificare in sala che plancia e proiettore funzionano prima di costruire il resto.

1. **Fase 0, approvazione.** Questo documento chiuso, risposte alle domande 1-4 della sezione 8.
2. **Fase 1, base + Modalità 1.** Due schede sincronizzate, concorrenti, timer, annulla, blackout, Estas o non Estas completa. Prova tecnica con il proiettore.
3. **Fase 2, modalità a scelta.** Date, La Scossa e Triello (strutture simili: domanda + alternative).
4. **Fase 3, modalità a tempo.** ESA, Centos, Ghigliottina, Duelli (timer e animazioni più delicati).
5. **Fase 4, rifinitura.** Grafica definitiva, suoni, punteggi, importazione contenuti da foglio di calcolo, prova generale.
