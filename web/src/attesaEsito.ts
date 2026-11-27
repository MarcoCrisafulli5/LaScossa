// Suspense dell'esito (RC-06): quando il conduttore conferma, l'opzione passa da "selezionata"
// a "giusta" o "sbagliata". Per attesaEsitoMs (da config.json, per modalità) il pubblico la mostra
// ancora gialla e la plancia tiene bloccato il passaggio all'oggetto successivo.
//
// È solo presentazione: il server ha già deciso e registrato l'esito. Per questo il ritardo vive
// nel browser e non nella macchina a stati. Una vista arrivata già giudicata (pagina ricaricata,
// riconnessione) non fa attendere: l'esito si vede subito.

import { useEffect, useRef, useState } from "react";
import type { StatoOpzione } from "./protocollo";

const giudicata = (s: StatoOpzione | undefined) => s === "giusta" || s === "sbagliata";

/**
 * Restituisce l'indice dell'opzione appena giudicata finché dura l'attesa, altrimenti null.
 * `chiave` identifica la domanda: se cambia (nuovo oggetto), l'attesa si azzera.
 */
export function useAttesaEsito(chiave: string, stati: StatoOpzione[], attesaMs: number): number | null {
  const [inAttesa, setInAttesa] = useState<number | null>(null);
  const precedente = useRef<{ chiave: string; stati: StatoOpzione[] } | null>(null);
  const firma = stati.join(",");

  useEffect(() => {
    const prec = precedente.current;
    precedente.current = { chiave, stati };
    const i =
      prec !== null && prec.chiave === chiave && attesaMs > 0
        ? stati.findIndex((s, k) => giudicata(s) && prec.stati[k] === "selezionata")
        : -1;
    if (i < 0) {
      // Qualsiasi altro cambiamento delle opzioni (annulla, nuova selezione, rivela) chiude l'attesa.
      setInAttesa(null);
      return;
    }
    setInAttesa(i);
    const t = setTimeout(() => setInAttesa(null), attesaMs);
    return () => clearTimeout(t);
    // `stati` è rappresentato da `firma`: l'effetto riparte solo se le opzioni cambiano davvero,
    // non a ogni vista (riquadro, turno, debug) che arriva durante l'attesa.
  }, [chiave, firma, attesaMs]);

  return inAttesa;
}
