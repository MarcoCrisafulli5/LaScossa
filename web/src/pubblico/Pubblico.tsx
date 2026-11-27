// Schermo pubblico sul proiettore: disegna la vista che riceve, senza logica propria.
// Non riceve mai soluzioni: il server gliele nasconde (VistaPubblico non ha campi per la soluzione).

import { useEffect } from "react";
import { useAttesaEsito } from "../attesaEsito";
import { BarraTimer } from "../BarraTimer";
import { useConnessione } from "../connessione";
import { trascorso, useOraServer } from "../orologio";
import type { EsaPubblico, EstasPubblico } from "../protocollo";

export function Pubblico() {
  const { vista } = useConnessione("pubblico");

  useEffect(() => {
    document.title = "La Scossa";
  }, []);

  // Prima della connessione, o se cade, si resta sull'ultima immagine o sul logo: niente errori in sala.
  if (!vista || vista.blackout) {
    return (
      <div className="pubblico">
        <div className={`palco${vista?.blackout ? " nero" : ""}`}>
          <div className="logo">LA SCOSSA</div>
        </div>
      </div>
    );
  }

  return (
    <div className="pubblico">
      <div className="palco">
        {vista.estas ? (
          <Estas e={vista.estas} />
        ) : vista.esa ? (
          <Esa e={vista.esa} />
        ) : (
          <div className="logo">LA SCOSSA</div>
        )}
        {vista.riquadro.visibile && vista.riquadro.testo && (
          <div className="riquadro-pubblico">{vista.riquadro.testo}</div>
        )}
      </div>
    </div>
  );
}

function Estas({ e }: { e: EstasPubblico }) {
  // Suspense: l'opzione appena confermata resta gialla per attesaEsitoMs, poi verde o rossa.
  const inAttesa = useAttesaEsito(
    `${e.categoria}|${e.oggetto}`,
    e.opzioni.map((o) => o.stato),
    e.attesaEsitoMs,
  );
  // Tra un oggetto e l'altro solo la categoria, grande: i conduttori possono spiegare regole e dubbi
  // senza la domanda a schermo.
  if (e.stato === "AttesaOggetto") return <div className="categoria-sola">{e.categoria}</div>;
  if (e.stato !== "OggettoInGioco") return <div className="categoria-sola">Estas o non Estas</div>;
  return (
    <>
      <div className="categoria-pubblico">{e.categoria || "Estas o non Estas"}</div>
      <div className="oggetto-pubblico">{e.oggetto}</div>
      {e.opzioni.length > 0 ? (
        <div className="opzioni-pubblico">
          {e.opzioni.map((o, i) => (
            <div key={o.etichetta} className={`opzione ${i === inAttesa ? "selezionata" : o.stato}`}>
              {o.etichetta}
            </div>
          ))}
        </div>
      ) : (
        <div />
      )}
    </>
  );
}

function Esa({ e }: { e: EsaPubblico }) {
  const ora = useOraServer(e.timer.stato === "corre");
  if (e.stato === "AttesaFrase" || e.stato === "Finita" || e.indizi.length === 0) {
    return <div className="categoria-sola">ESA</div>;
  }
  // Gli indizi compaiono uno ogni letturaIndizioMs seguendo il timer (si fermano con la pausa);
  // a frase conclusa si vedono tutti.
  const visibili =
    e.stato === "InGioco"
      ? Math.min(e.indizi.length, Math.floor(trascorso(e.timer, ora) / e.letturaIndizioMs) + 1)
      : e.indizi.length;
  return (
    <>
      <div className="categoria-pubblico">{e.stato === "Scaduta" ? "Tempo scaduto" : "ESA"}</div>
      <div className="esa-centro">
        <div className="indizi-pubblico">
          {e.indizi.map((indizio, i) => (
            <div key={i} className={`indizio${i < visibili ? " visibile" : ""}`}>
              {i < visibili ? indizio : ""}
            </div>
          ))}
        </div>
        {e.frase && <div className={`frase-pubblico ${e.stato.toLowerCase()}`}>{e.frase}</div>}
      </div>
      <BarraTimer t={e.timer} ora={ora} classe="barra-pubblico" />
    </>
  );
}
