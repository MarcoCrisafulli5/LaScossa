// Prova end-to-end del protocollo: una plancia e un pubblico veri via WebSocket contro un server
// avviato su una partita NUOVA (Lobby) con i dati di esempio di dati/.
//
//   ./lascossa-server --dati <copia di dati/> --web web/dist --porta 8099 --nuova
//   node server/test/e2e.mjs [porta]
//
// Usa una copia di dati/: la prova scrive un registro in <dati>/partite/.
// Richiede Node 22 o successivo (WebSocket e fetch globali).

const PORTA = process.argv[2] ?? "8099";
const WS = `ws://127.0.0.1:${PORTA}/ws`;
const HTTP = `http://127.0.0.1:${PORTA}`;
const attendi = (ms) => new Promise((r) => setTimeout(r, ms));

function client(ruolo) {
  const ws = new WebSocket(WS);
  const c = { ws, ricevuti: [], vista: null };
  ws.onmessage = (e) => {
    const m = JSON.parse(e.data);
    c.ricevuti.push(m);
    if (m.vista) c.vista = m.vista;
  };
  return new Promise((ok, ko) => {
    ws.onerror = () => ko(new Error(`server non raggiungibile su ${WS}`));
    ws.onopen = () => {
      ws.send(JSON.stringify({ tipo: "hello", ruolo }));
      ok(c);
    };
  });
}

let n = 0;
async function invia(p, evento, arg = -1, testo = "") {
  const id = "e2e-" + ++n;
  const prima = p.ricevuti.length;
  const t0 = performance.now();
  p.ws.send(JSON.stringify({ tipo: "event", id, evento, arg, testo }));
  while (!p.ricevuti.slice(prima).some((m) => m.id === id)) await attendi(5);
  const esito = p.ricevuti.slice(prima).find((m) => m.id === id);
  await attendi(30); // lascia arrivare gli update
  return { ...esito, ms: performance.now() - t0 };
}

const fallimenti = [];
const verifica = (cond, msg) => {
  console.log((cond ? "OK   " : "FAIL ") + msg);
  if (!cond) fallimenti.push(msg);
};

const plancia = await client("plancia");
const pubblico = await client("pubblico");
await attendi(200);
verifica(plancia.vista?.statoSerata === "Lobby", "snapshot plancia: Lobby (avviare il server con --nuova)");
verifica(pubblico.vista?.modalita === null, "pubblico: schermata di attesa");

let r = await invia(plancia, "AVVIA_SERATA");
verifica(r.tipo === "ack" && r.ms < 500, `AVVIA_SERATA ack in ${r.ms.toFixed(1)} ms (requisito RC-01: < 500 ms)`);
r = await invia(plancia, "SCEGLI_MODALITA", 2);  // Date: non ancora implementata
verifica(r.tipo === "reject", `modalità non implementata rifiutata: "${r.motivo}"`);
await invia(plancia, "SCEGLI_MODALITA", 0);
const cat = plancia.vista.estas.categorie.length - 1;
await invia(plancia, "SCEGLI_CATEGORIA", cat);
const inArrivo = plancia.vista.estas.anteprima;
verifica(
  pubblico.vista.estas.stato === "AttesaOggetto" && pubblico.vista.estas.oggetto === "" && pubblico.vista.estas.categoria !== "",
  "attesa: il pubblico vede solo la categoria",
);
verifica(typeof inArrivo === "string" && !JSON.stringify(pubblico.ricevuti).includes(`"${inArrivo}"`),
  `attesa: "${inArrivo}" è in anteprima in plancia ma non arriva al pubblico`);
await invia(plancia, "MOSTRA_DOMANDA");
const oggetto = plancia.vista.estas.oggetto;
const giusta = plancia.vista.estas.oggettoAppartiene ? 0 : 1;
const sbagliata = 1 - giusta;
verifica(typeof oggetto === "string" && pubblico.vista.estas.oggetto === oggetto, `pubblico vede l'oggetto "${oggetto}"`);
verifica(!JSON.stringify(pubblico.ricevuti).match(/appartiene|corretta/), "pubblico: nessun campo con la soluzione");
verifica(Number.isInteger(pubblico.vista.estas.attesaEsitoMs) &&
         pubblico.vista.estas.attesaEsitoMs === plancia.vista.estas.attesaEsitoMs,
  `attesa dell'esito da config uguale su plancia e pubblico (${pubblico.vista.estas.attesaEsitoMs} ms)`);

await invia(plancia, "SELEZIONA", sbagliata);
verifica(pubblico.vista.estas.opzioni[sbagliata].stato === "selezionata", "selezione in giallo");
await invia(plancia, "CONFERMA");
verifica(pubblico.vista.estas.opzioni[sbagliata].stato === "sbagliata", "risposta sbagliata in rosso");
verifica(pubblico.vista.estas.opzioni[giusta].stato === "neutra", "la giusta resta nascosta");
await invia(plancia, "ANNULLA");
verifica(pubblico.vista.estas.opzioni[sbagliata].stato === "selezionata", "annulla: torna giallo");
await invia(plancia, "ANNULLA");
await invia(plancia, "SELEZIONA", giusta);
await invia(plancia, "CONFERMA");
verifica(pubblico.vista.estas.opzioni[giusta].stato === "giusta", "risposta giusta in verde");
await invia(plancia, "PROSSIMA_DOMANDA");
verifica(pubblico.vista.estas.stato === "AttesaOggetto", "prossimo oggetto: prima di nuovo solo la categoria");
await invia(plancia, "MOSTRA_DOMANDA");
await invia(plancia, "RIVELA");
verifica(pubblico.vista.estas.opzioni.some((o) => o.stato === "soluzione"), "rivela mostra la soluzione");

await invia(plancia, "AGGIORNA_RIQUADRO", -1, "Anna 1000 – Bruno 500");
await invia(plancia, "MOSTRA_RIQUADRO");
verifica(pubblico.vista.riquadro.visibile && pubblico.vista.riquadro.testo.includes("Anna"), "riquadro punteggio");
await invia(plancia, "BLACKOUT_ON");
verifica(pubblico.vista.blackout, "blackout");
await invia(plancia, "BLACKOUT_OFF");
await invia(plancia, "PAUSA_GENERALE");
r = await invia(plancia, "PROSSIMA_DOMANDA");
verifica(r.tipo === "reject", `in pausa gli eventi di gioco sono rifiutati: "${r.motivo}"`);
await invia(plancia, "RIPRENDI_GENERALE");
r = await invia(plancia, "NUOVA_PARTITA");
verifica(r.tipo === "reject", "nuova partita rifiutata durante il gioco");

// --- ESA: indizi e timer di gioco del server; la frase al pubblico solo dopo
await invia(plancia, "FINE_MODALITA");
r = await invia(plancia, "SCEGLI_MODALITA", 1);
verifica(r.tipo === "ack" && pubblico.vista.esa?.stato === "AttesaFrase", "ESA: in attesa il pubblico vede solo il titolo");
const soluzioneEsa = plancia.vista.esa.anteprima.soluzione;
verifica(pubblico.vista.esa.indizi.length === 0, "ESA: in attesa nessun indizio al pubblico");
r = await invia(plancia, "MOSTRA_DOMANDA");
const t = pubblico.vista.esa.timer;
const oraMostra = pubblico.ricevuti.at(-1).oraServer;
verifica(pubblico.vista.esa.indizi.length === 3 && t.stato === "corre", "ESA: tre indizi e timer che corre");
verifica(t.scadenzaMs - oraMostra === t.totaleMs && t.totaleMs - t.durataMs === 3 * pubblico.vista.esa.letturaIndizioMs,
  `ESA: il conto alla rovescia (${t.durataMs / 1000} s) parte dopo i 3 indizi`);
verifica(!JSON.stringify(pubblico.ricevuti).includes(soluzioneEsa), "ESA: la frase non arriva al pubblico prima del tempo");
r = await invia(plancia, "TIMER_SCADUTO");
verifica(r.tipo === "reject", `ESA: la plancia non può far scadere il timer: "${r.motivo}"`);
r = await invia(plancia, "CONFERMA_SBAGLIATA");
verifica(r.tipo === "reject", "ESA: una risposta sbagliata non chiude la frase");
await invia(plancia, "PAUSA_GENERALE");
verifica(pubblico.vista.esa.timer.stato === "pausa", "ESA: la pausa generale ferma il timer");
await invia(plancia, "RIPRENDI_GENERALE");
await invia(plancia, "RIVELA");
verifica(pubblico.vista.esa.frase === soluzioneEsa && pubblico.vista.esa.timer.stato === "fermo",
  "ESA: rivela mostra la frase e ferma il timer");

plancia.ws.send(JSON.stringify({ tipo: "ping" }));
await attendi(100);
verifica(plancia.ricevuti.some((m) => m.tipo === "pong"), "ping/pong");

const pagina = await fetch(`${HTTP}/plancia`);
verifica(pagina.ok && (await pagina.text()).includes('id="app"'), "HTTP /plancia serve l'interfaccia");
verifica((await fetch(`${HTTP}/assets/non-esiste.js`)).status === 404, "HTTP file mancante: 404");
const fuga = await fetch(`${HTTP}/..%2F..%2Fdati%2Fconfig.json`);
verifica(fuga.status === 404 || !(await fuga.text()).includes("concorrenti"), "HTTP: niente file fuori da web/dist");

plancia.ws.close();
pubblico.ws.close();
console.log(fallimenti.length ? `\n${fallimenti.length} FALLITI` : "\nTUTTO OK");
process.exit(fallimenti.length ? 1 : 0);
