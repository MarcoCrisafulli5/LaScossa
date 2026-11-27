// Modalità 2 in plancia: frase in arrivo con soluzione, indizi (con quelli già sul pubblico),
// timer di gioco, pulsanti "segna" (risposta giusta) separati da quelli "mostra" (rivela, prossima).
// Una risposta sbagliata non si segna: il concorrente riprova finché c'è tempo.

import { BarraTimer } from "../BarraTimer";
import { trascorso, useOraServer } from "../orologio";
import { ammesso, type EsaPlancia as Esa, type FrasePlancia, type VistaPlancia } from "../protocollo";
import type { Invia } from "./Plancia";

const ESITI: Partial<Record<Esa["stato"], string>> = {
  Indovinata: "Indovinata",
  Rivelata: "Rivelata dal conduttore",
  Scaduta: "Tempo scaduto: frase rivelata",
};

export function EsaPlancia({ v, e, invia }: { v: VistaPlancia; e: Esa; invia: Invia }) {
  const ora = useOraServer(e.timer.stato === "corre");
  const sulPubblico =
    e.stato === "InGioco" ? Math.min(3, Math.floor(trascorso(e.timer, ora) / e.letturaIndizioMs) + 1) : 3;

  if (e.stato === "Finita") {
    return <p>Frasi finite ({e.totale}): chiudi la modalità con "Fine modalità".</p>;
  }

  return (
    <div className="esa">
      <p className="tenue" style={{ margin: 0 }}>
        Frase {e.stato === "AttesaFrase" ? e.giocate + 1 : e.giocate} di {e.totale} · stato: <b>{e.stato}</b>
        {e.stato === "AttesaFrase" && (
          <>
            {" "}
            · <b>il pubblico vede solo il titolo</b>
          </>
        )}
      </p>

      {e.stato === "AttesaFrase" && e.anteprima && (
        <>
          <Frase f={e.anteprima} visibili={0} />
          <div className="azioni" style={{ marginTop: "1.2rem" }}>
            <button className="primario" disabled={!ammesso(v, "MOSTRA_DOMANDA")} onClick={() => invia("MOSTRA_DOMANDA")}>
              Mostra indizi e avvia il timer
            </button>
          </div>
        </>
      )}

      {e.frase && (
        <>
          <Frase f={e.frase} visibili={sulPubblico} />
          <BarraTimer t={e.timer} ora={ora} />
          {ESITI[e.stato] && <p className={`esito-esa ${e.stato.toLowerCase()}`}>{ESITI[e.stato]}</p>}

          <div className="gruppi">
            <div>
              <h3 style={{ marginTop: 0 }}>Segna</h3>
              <div className="azioni">
                <button className="primario" disabled={!ammesso(v, "CONFERMA_GIUSTA")} onClick={() => invia("CONFERMA_GIUSTA")}>
                  {e.stato === "Scaduta" ? "Era giusta (allo scadere)" : "Risposta giusta"}
                </button>
              </div>
              <p className="tenue" style={{ fontSize: "0.85em" }}>
                Risposta sbagliata: non serve segnarla, il concorrente riprova finché c'è tempo.
              </p>
            </div>
            <div>
              <h3 style={{ marginTop: 0 }}>Mostra</h3>
              <div className="azioni">
                <button disabled={!ammesso(v, "RIVELA")} onClick={() => invia("RIVELA")}>
                  Rivela frase
                </button>
                <button disabled={!ammesso(v, "PROSSIMA_DOMANDA")} onClick={() => invia("PROSSIMA_DOMANDA")}>
                  Prossima frase (prima il titolo)
                </button>
              </div>
            </div>
          </div>
        </>
      )}
    </div>
  );
}

// Indizi e soluzione: la plancia li vede sempre; `visibili` indica quanti indizi ha già il pubblico.
function Frase({ f, visibili }: { f: FrasePlancia; visibili: number }) {
  return (
    <>
      <ol className="indizi-plancia">
        {f.indizi.map((indizio, i) => (
          <li key={i} className={i < visibili ? "sul-pubblico" : undefined}>
            {indizio}
            {i < visibili && <span className="tenue"> · sul pubblico</span>}
          </li>
        ))}
      </ol>
      <span className="soluzione-plancia">Soluzione: {f.soluzione}</span>
    </>
  );
}
