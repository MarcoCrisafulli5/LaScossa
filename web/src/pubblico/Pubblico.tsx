// Schermo pubblico sul proiettore: disegna la vista che riceve, senza logica propria.
// Non riceve mai soluzioni: il server gliele nasconde (VistaPubblico non ha campi per la soluzione).

import { useEffect } from "react";
import { useAttesaEsito } from "../attesaEsito";
import { useConnessione } from "../connessione";
import type { EstasPubblico } from "../protocollo";

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
        {vista.estas ? <Estas e={vista.estas} /> : <div className="logo">LA SCOSSA</div>}
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
