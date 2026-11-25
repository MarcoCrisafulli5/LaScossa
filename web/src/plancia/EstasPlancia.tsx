// Modalità 1 in plancia: categorie, oggetto con soluzione, risposta del concorrente,
// pulsanti "segna" (conferma) separati da quelli "mostra" (rivela, prossimo oggetto).

import { ammesso, type EstasPlancia as Estas, type VistaPlancia } from "../protocollo";
import type { Invia } from "./Plancia";

export function EstasPlancia({ v, e, invia }: { v: VistaPlancia; e: Estas; invia: Invia }) {
  return (
    <div className="estas">
      <div>
        <h3>Categorie</h3>
        <div className="categorie">
          {e.categorie.map((c, i) => (
            <button
              key={c.nome}
              className={i === e.categoriaCorrente ? "attivo" : undefined}
              disabled={!ammesso(v, "SCEGLI_CATEGORIA", i)}
              onClick={() => invia("SCEGLI_CATEGORIA", i)}
            >
              <span>{c.nome}</span>
              <span className="tenue">
                {c.giocati}/{c.totale}
              </span>
            </button>
          ))}
        </div>
        <p className="tenue" style={{ fontSize: "0.85em" }}>
          Si cambia categoria solo quando l'oggetto in gioco ha un esito.
        </p>
      </div>

      <div>
        {e.stato === "SceltaCategoria" && <p className="tenue">Scegli una categoria per iniziare.</p>}
        {e.stato === "CategoriaFinita" && (
          <p>
            Categoria <b>{e.categorie[e.categoriaCorrente]?.nome}</b> finita: scegline un'altra.
          </p>
        )}
        {e.oggetto !== null && (
          <>
            <p className="tenue" style={{ margin: 0 }}>
              {e.categorie[e.categoriaCorrente]?.nome} · oggetto {e.categorie[e.categoriaCorrente]?.giocati} di{" "}
              {e.categorie[e.categoriaCorrente]?.totale} · domanda: <b>{e.statoDomanda}</b>
            </p>
            <div className="oggetto">{e.oggetto}</div>
            <span className="soluzione-plancia">Soluzione: {e.oggettoAppartiene ? "ESTAS" : "NON ESTAS"}</span>

            <h3>Risposta del concorrente</h3>
            <div className="opzioni-plancia">
              {e.opzioni.map((o, i) => (
                <button
                  key={o.etichetta}
                  className={`opzione ${o.stato}`}
                  disabled={!ammesso(v, "SELEZIONA", i)}
                  onClick={() => invia("SELEZIONA", i)}
                >
                  {o.etichetta}
                  {o.corretta && <span className="corretta-segno">✓ giusta</span>}
                </button>
              ))}
            </div>

            <div className="gruppi">
              <div>
                <h3 style={{ marginTop: 0 }}>Segna</h3>
                <div className="azioni">
                  <button className="primario" disabled={!ammesso(v, "CONFERMA")} onClick={() => invia("CONFERMA")}>
                    Conferma risposta
                  </button>
                  <button disabled={!ammesso(v, "PASSA_TURNO")} onClick={() => invia("PASSA_TURNO")}>
                    Passa turno
                  </button>
                </div>
                <p className="turno">
                  Di turno: concorrente <b>{e.concorrenteDiTurno + 1}</b> di {e.numeroConcorrenti}
                </p>
              </div>
              <div>
                <h3 style={{ marginTop: 0 }}>Mostra</h3>
                <div className="azioni">
                  <button disabled={!ammesso(v, "RIVELA")} onClick={() => invia("RIVELA")}>
                    Rivela soluzione
                  </button>
                  <button disabled={!ammesso(v, "PROSSIMA_DOMANDA")} onClick={() => invia("PROSSIMA_DOMANDA")}>
                    Prossimo oggetto
                  </button>
                </div>
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
