// Pagina /debug (ruolo plancia): stato corrente, ultimi eventi del registro, eventi rifiutati
// e transizioni delle macchine a stati. Il diagramma delle FSM arriverà con lo schema JSON.

import { useEffect } from "react";
import { useConnessione } from "../connessione";

function ora(t: number): string {
  return t ? new Date(t).toLocaleTimeString("it-IT") : "";
}

export function Debug() {
  const { vista, stato } = useConnessione("plancia");

  useEffect(() => {
    document.title = "La Scossa · Debug";
  }, []);

  if (!vista) return <p className="debug">Connessione: {stato}…</p>;
  const d = vista.debug;
  const ammessi = vista.ammessi.map((a) => (a.arg >= 0 ? `${a.evento}(${a.arg})` : a.evento)).join(", ");

  return (
    <div className="debug">
      <section className="pannello">
        <h2>Stato</h2>
        <p>
          Connessione: <b>{stato}</b> · Serata: <b>{vista.statoSerata}</b>
          {vista.modalitaAttiva && (
            <>
              {" "}· {vista.modalitaAttiva.nome}: <b>{vista.statoModalita}</b>
            </>
          )}
          {vista.estas?.statoDomanda && (
            <>
              {" "}· Domanda: <b>{vista.estas.statoDomanda}</b>
            </>
          )}
        </p>
        <p>
          Registro: {vista.server.registro} · {d.lunghezzaRegistro} eventi
        </p>
        <p className="tenue">Eventi ammessi: {ammessi || "nessuno"}</p>
      </section>

      <div className="colonne">
        <section className="pannello">
          <h2>Ultimi eventi accettati</h2>
          <table>
            <thead>
              <tr><th>#</th><th>Ora</th><th>Evento</th><th>Arg</th><th>Testo</th></tr>
            </thead>
            <tbody>
              {[...d.ultimiEventi].reverse().map((e) => (
                <tr key={e.seq}>
                  <td>{e.seq}</td><td>{ora(e.t)}</td><td>{e.evento}</td><td>{e.arg >= 0 ? e.arg : ""}</td><td>{e.testo}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </section>

        <section className="pannello">
          <h2>Transizioni</h2>
          <table>
            <thead>
              <tr><th>Macchina</th><th>Da</th><th>Evento</th><th>A</th></tr>
            </thead>
            <tbody>
              {[...d.ultimeTransizioni].reverse().map((t, i) => (
                <tr key={i}>
                  <td>{t.macchina}</td><td>{t.da}</td><td>{t.evento}</td><td>{t.a}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </section>
      </div>

      <section className="pannello">
        <h2>Eventi rifiutati</h2>
        <table>
          <thead>
            <tr><th>Ora</th><th>Evento</th><th>Arg</th><th>Motivo</th></tr>
          </thead>
          <tbody>
            {[...d.ultimiRifiutati].reverse().map((r, i) => (
              <tr key={i}>
                <td>{ora(r.t)}</td><td>{r.evento}</td><td>{r.arg >= 0 ? r.arg : ""}</td><td>{r.motivo}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </section>

      <details className="pannello">
        <summary>Vista plancia completa (JSON)</summary>
        <pre>{JSON.stringify(vista, null, 2)}</pre>
      </details>
    </div>
  );
}
