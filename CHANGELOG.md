# Changelog

Tutte le modifiche di La Scossa, dalla più recente. Formato ispirato a
[Keep a Changelog](https://keepachangelog.com/it-IT/1.1.0/), versioni [SemVer](https://semver.org/lang/it/).

La versione sta nel file `VERSION`; l'immagine Docker ha lo stesso tag (`lascossa:<versione>`).
Ogni modifica aggiunge una riga in **Non rilasciato**. Al rilascio quella sezione prende numero e data.

## [Non rilasciato]

## [0.1.0] - 2026-11-27

Prima versione giocabile: la serata con la modalità 1.

### Aggiunto

- Serata: lobby, scelta libera della modalità, pausa generale, blackout, riquadro punteggio a comparsa.
- Modalità 1 **Estas o non Estas**: categorie, attesa prima di ogni oggetto (il pubblico vede solo la
  categoria, la plancia l'oggetto in arrivo), regola giallo → verde/rosso, rivela soluzione, turni.
- Suspense dell'esito: dopo la conferma il pubblico tiene il giallo per `estas.attesaEsitoMs`
  (default 2000 ms) e poi colora; intanto in plancia "Prossimo oggetto" e il cambio categoria sono spenti.
- Annulla ultima azione; registro eventi in `dati/partite/*.jsonl` e ripresa dopo un crash; nuova partita.
- Server C++20 (Crow, WebSocket) con plancia, pubblico e pagina `/debug` in React.
- Contenuti da CSV e parametri da `dati/config.json`, con errori che indicano file e riga.
- Immagine Docker multi-stage che compila e testa tutto; versione nel file `VERSION`.

### Corretto

- Sul proiettore la risposta scelta e il suo esito (giallo, verde, rosso) non si vedevano: una regola
  CSS del pubblico copriva i colori degli stati. Ora si vedono come in plancia.

### Documentazione

- Analisi dei requisiti e del problema/architettura.
- Duelli analizzati come seconda partita accanto alla serata (RC-15 … RC-20); regole di gioco rimandate.
- Due tipi di timer (RC-05): timer di gioco nel server, timer liberi creati dalla plancia.

[Non rilasciato]: https://github.com/MarcoCrisafulli5/LaScossa/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/MarcoCrisafulli5/LaScossa/releases/tag/v0.1.0
