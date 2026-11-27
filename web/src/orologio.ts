// Ora del server nel browser, per disegnare i timer di gioco (RC-05).
//
// Ogni messaggio del server porta oraServer. Lo scarto con l'orologio del browser si corregge solo
// se cambia più di SOGLIA_MS (documento di architettura: "orologi di server e browser non allineati"),
// così la latenza di rete non fa tremare il conto alla rovescia.

import { useEffect, useState } from "react";
import type { TimerVista } from "./protocollo";

const SOGLIA_MS = 250;
const PASSO_MS = 100; // ogni quanto si ridisegna un timer che corre

let scarto = 0;
let allineato = false;

export function aggiornaOrologio(oraServer: number) {
  const nuovo = oraServer - Date.now();
  if (!allineato || Math.abs(nuovo - scarto) > SOGLIA_MS) {
    scarto = nuovo;
    allineato = true;
  }
}

export const oraServer = () => Date.now() + scarto;

/** Ora del server, aggiornata ogni PASSO_MS finché `attivo` (es. un timer che corre). */
export function useOraServer(attivo: boolean): number {
  const [ora, setOra] = useState(oraServer);
  useEffect(() => {
    setOra(oraServer());
    if (!attivo) return;
    const id = window.setInterval(() => setOra(oraServer()), PASSO_MS);
    return () => window.clearInterval(id);
  }, [attivo]);
  return ora;
}

/** Millisecondi alla scadenza, ritardo iniziale compreso. */
export function mancaAllaFine(t: TimerVista, ora: number): number {
  return t.stato === "corre" ? Math.max(0, t.scadenzaMs - ora) : t.residuoMs;
}

/** Millisecondi del conto alla rovescia visibile (al massimo la durata). */
export function mancaAlConto(t: TimerVista, ora: number): number {
  return Math.min(t.durataMs, mancaAllaFine(t, ora));
}

/** Millisecondi trascorsi dall'avvio del timer (serve agli indizi di ESA): si ferma con la pausa. */
export function trascorso(t: TimerVista, ora: number): number {
  return t.totaleMs - mancaAllaFine(t, ora);
}
