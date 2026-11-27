// Plancia di comando dei conduttori. Mostra lo stato e abilita solo i pulsanti degli eventi
// che il server accetterebbe (vista.ammessi): nessuna regola di gioco è scritta qui.

import { useEffect, useState, type ReactNode } from "react";
import { useConnessione, type StatoConnessione } from "../connessione";
import { ammesso, type NomeEvento, type VistaPlancia } from "../protocollo";
import { EsaPlancia } from "./EsaPlancia";
import { EstasPlancia } from "./EstasPlancia";

const DURATA_MESSAGGIO_RIFIUTO_MS = 6000;

export type Invia = (evento: NomeEvento, arg?: number, testo?: string) => void;

export function Plancia() {
  const c = useConnessione("plancia");
  const [rifiutoVisibile, setRifiutoVisibile] = useState(false);

  useEffect(() => {
    document.title = "La Scossa · Plancia";
  }, []);

  useEffect(() => {
    if (!c.rifiuto) return;
    setRifiutoVisibile(true);
    const t = window.setTimeout(() => setRifiutoVisibile(false), DURATA_MESSAGGIO_RIFIUTO_MS);
    return () => window.clearTimeout(t);
  }, [c.rifiuto]);

  if (c.pinRichiesto) return <RichiestaPin onPin={c.impostaPin} errore={c.rifiuto?.motivo} />;
  if (!c.vista) {
    return (
      <div className="pin">
        <Connessione stato={c.stato} />
        <p className="tenue">In attesa del server…</p>
      </div>
    );
  }

  const v = c.vista;
  return (
    <div className="plancia">
      <Barra v={v} stato={c.stato} invia={c.invia} />
      <div className="messaggi">
        {rifiutoVisibile && c.rifiuto && <div className="messaggio-rifiuto">Non eseguito: {c.rifiuto.motivo}</div>}
        {v.pausa && <div className="messaggio-pausa">PAUSA GENERALE: gli eventi di gioco sono bloccati</div>}
        {v.blackout && <div className="messaggio-blackout">Blackout attivo: il pubblico vede solo il logo</div>}
      </div>
      <div className="contenuto">
        <main>
          {v.statoSerata === "Lobby" && <Lobby v={v} invia={c.invia} />}
          {v.statoSerata === "SceltaModalita" && <SceltaModalita v={v} invia={c.invia} />}
          {v.statoSerata === "InGioco" && <InGioco v={v} invia={c.invia} />}
          {v.statoSerata === "Chiusa" && <Chiusa v={v} invia={c.invia} />}
        </main>
        <RiquadroPunteggio v={v} invia={c.invia} />
      </div>
      <Piede v={v} />
    </div>
  );
}

function Connessione({ stato }: { stato: StatoConnessione }) {
  return <span className={`connessione ${stato === "connesso" ? "connesso" : stato}`}>{stato}</span>;
}

function Pulsante(props: {
  v: VistaPlancia;
  invia: Invia;
  evento: NomeEvento;
  arg?: number;
  className?: string;
  children: ReactNode;
}) {
  const arg = props.arg ?? -1;
  return (
    <button
      className={props.className}
      disabled={!ammesso(props.v, props.evento, arg)}
      onClick={() => props.invia(props.evento, arg)}
    >
      {props.children}
    </button>
  );
}

function Barra({ v, stato, invia }: { v: VistaPlancia; stato: StatoConnessione; invia: Invia }) {
  return (
    <header className="barra">
      <span className="marchio">LA SCOSSA · PLANCIA</span>
      <Connessione stato={stato} />
      <span className="stato">
        Serata: <b>{v.statoSerata}</b>
        {v.modalitaAttiva && (
          <>
            {" "}· {v.modalitaAttiva.nome}: <b>{v.statoModalita}</b>
          </>
        )}
      </span>
      <span className="spazio" />
      <Pulsante v={v} invia={invia} evento="ANNULLA">
        ↶ Annulla ultima azione
      </Pulsante>
      {v.pausa ? (
        <Pulsante v={v} invia={invia} evento="RIPRENDI_GENERALE" className="attivo">
          ▶ Riprendi
        </Pulsante>
      ) : (
        <Pulsante v={v} invia={invia} evento="PAUSA_GENERALE">
          ❚❚ Pausa generale
        </Pulsante>
      )}
      {v.blackout ? (
        <Pulsante v={v} invia={invia} evento="BLACKOUT_OFF" className="attivo">
          Togli blackout
        </Pulsante>
      ) : (
        <Pulsante v={v} invia={invia} evento="BLACKOUT_ON">
          Blackout
        </Pulsante>
      )}
    </header>
  );
}

function Lobby({ v, invia }: { v: VistaPlancia; invia: Invia }) {
  return (
    <section className="pannello">
      <h2>Preparazione della serata</h2>
      <p>
        Concorrenti: <b>{v.numeroConcorrenti}</b> <span className="tenue">(da config.json)</span>
      </p>
      <p>
        Estas o non Estas: <b>{v.contenuti.estasCategorie}</b> categorie, <b>{v.contenuti.estasOggetti}</b> oggetti{" "}
        <span className="tenue">(da contenuti/mod1_estas.csv)</span>
      </p>
      <Avvisi avvisi={v.server.avvisi} />
      <div className="azioni" style={{ marginTop: "1rem" }}>
        <Pulsante v={v} invia={invia} evento="AVVIA_SERATA" className="primario">
          Avvia serata
        </Pulsante>
        <Pulsante v={v} invia={invia} evento="NUOVA_PARTITA">
          Nuova partita (ricarica dati)
        </Pulsante>
      </div>
    </section>
  );
}

function SceltaModalita({ v, invia }: { v: VistaPlancia; invia: Invia }) {
  return (
    <section className="pannello">
      <h2>Scegli la modalità</h2>
      <div className="griglia-modalita">
        {v.modalita.map((m) => (
          <Pulsante key={m.id} v={v} invia={invia} evento="SCEGLI_MODALITA" arg={m.id}>
            {m.id + 1}. {m.nome}
            <small>{!m.disponibile ? "non ancora disponibile" : m.giocata ? "già giocata" : ""}</small>
          </Pulsante>
        ))}
      </div>
      <div className="azioni" style={{ marginTop: "1rem" }}>
        <Pulsante v={v} invia={invia} evento="CHIUDI_SERATA" className="pericolo">
          Chiudi serata
        </Pulsante>
      </div>
    </section>
  );
}

function InGioco({ v, invia }: { v: VistaPlancia; invia: Invia }) {
  return (
    <section className="pannello">
      <div className="azioni" style={{ justifyContent: "space-between", alignItems: "center", marginBottom: "1rem" }}>
        <h2 style={{ margin: 0 }}>{v.modalitaAttiva?.nome}</h2>
        <Pulsante v={v} invia={invia} evento="FINE_MODALITA">
          Fine modalità
        </Pulsante>
      </div>
      {v.estas ? (
        <EstasPlancia v={v} e={v.estas} invia={invia} />
      ) : v.esa ? (
        <EsaPlancia v={v} e={v.esa} invia={invia} />
      ) : (
        <p className="tenue">Modalità senza plancia.</p>
      )}
    </section>
  );
}

function Chiusa({ v, invia }: { v: VistaPlancia; invia: Invia }) {
  return (
    <section className="pannello">
      <h2>Serata chiusa</h2>
      <p>
        Modalità giocate:{" "}
        {v.modalita
          .filter((m) => m.giocata)
          .map((m) => m.nome)
          .join(", ") || "nessuna"}
      </p>
      <Pulsante v={v} invia={invia} evento="NUOVA_PARTITA" className="primario">
        Nuova partita
      </Pulsante>
    </section>
  );
}

function RiquadroPunteggio({ v, invia }: { v: VistaPlancia; invia: Invia }) {
  const [testo, setTesto] = useState(v.riquadro.testo);
  // Se il testo cambia dal server (annulla, altra plancia) il campo si riallinea.
  useEffect(() => setTesto(v.riquadro.testo), [v.riquadro.testo]);
  const modificato = testo !== v.riquadro.testo;
  return (
    <aside className="pannello riquadro-plancia">
      <h2>Riquadro punteggio</h2>
      <textarea value={testo} onChange={(e) => setTesto(e.target.value)} placeholder="Es. Anna 1000 – Bruno 500" />
      <div className="azioni" style={{ marginTop: "0.5rem" }}>
        <button disabled={!modificato || !ammesso(v, "AGGIORNA_RIQUADRO")} onClick={() => invia("AGGIORNA_RIQUADRO", -1, testo)}>
          Aggiorna testo
        </button>
        {v.riquadro.visibile ? (
          <Pulsante v={v} invia={invia} evento="NASCONDI_RIQUADRO" className="attivo">
            Nascondi
          </Pulsante>
        ) : (
          <Pulsante v={v} invia={invia} evento="MOSTRA_RIQUADRO">
            Mostra
          </Pulsante>
        )}
      </div>
      <p className="anteprima tenue">
        Sul pubblico: {v.riquadro.visibile ? "visibile" : "nascosto"}
        {v.riquadro.testo ? ` · "${v.riquadro.testo}"` : " · vuoto"}
      </p>
    </aside>
  );
}

function Avvisi({ avvisi }: { avvisi: string[] }) {
  if (avvisi.length === 0) return null;
  return (
    <ul className="lista-avvisi">
      {avvisi.map((a) => (
        <li key={a}>{a}</li>
      ))}
    </ul>
  );
}

function Piede({ v }: { v: VistaPlancia }) {
  return (
    <footer className="piede">
      <span>Registro: {v.server.registro} ({v.debug.lunghezzaRegistro} eventi)</span>
      <a href="/pubblico" target="_blank" rel="noreferrer">
        Apri schermo pubblico
      </a>
      <a href="/debug" target="_blank" rel="noreferrer">
        Debug
      </a>
    </footer>
  );
}

function RichiestaPin({ onPin, errore }: { onPin: (pin: string) => void; errore?: string }) {
  const [pin, setPin] = useState("");
  return (
    <form
      className="pin pannello"
      onSubmit={(e) => {
        e.preventDefault();
        onPin(pin);
      }}
    >
      <h2>PIN della plancia</h2>
      <input type="password" inputMode="numeric" autoFocus value={pin} onChange={(e) => setPin(e.target.value)} />
      {errore && <span className="tenue">{errore}</span>}
      <button className="primario" type="submit">
        Entra
      </button>
    </form>
  );
}
