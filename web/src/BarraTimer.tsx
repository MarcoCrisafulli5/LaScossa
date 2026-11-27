// Timer di gioco in forma grafica: una barra che si accorcia e i secondi che mancano.
// Uguale su plancia e pubblico; colori e misure da stile.css. Prima che il conto alla rovescia
// parta (ritardo iniziale, es. gli indizi di ESA) la barra resta piena e ferma.

import { mancaAlConto } from "./orologio";
import type { TimerVista } from "./protocollo";

export function BarraTimer({ t, ora, classe = "" }: { t: TimerVista; ora: number; classe?: string }) {
  if (t.stato === "spento") return null;
  const manca = mancaAlConto(t, ora);
  const frazione = t.durataMs > 0 ? manca / t.durataMs : 0;
  const secondi = Math.ceil(manca / 1000);
  const finale = t.stato === "corre" && manca <= 5000 && manca < t.durataMs; // ultimi 5 s
  return (
    <div className={`barra-timer ${t.stato}${finale ? " finale" : ""} ${classe}`}>
      <div className="barra-timer-pieno" style={{ width: `${frazione * 100}%` }} />
      <span className="barra-timer-secondi">{t.stato === "pausa" ? `❚❚ ${secondi}` : secondi}</span>
    </div>
  );
}
