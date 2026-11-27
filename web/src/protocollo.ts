// Tipi dei messaggi tra server e browser.
//
// Specchio a mano di server/src/json_viste.cpp e server/src/regia.cpp: se cambi uno dei due
// lati, cambia anche l'altro. L'architettura prevede di generarli entrambi da uno schema JSON
// unico (schema/), non ancora realizzato.

export type NomeEvento =
  | "AVVIA_SERATA"
  | "SCEGLI_MODALITA"
  | "FINE_MODALITA"
  | "CHIUDI_SERATA"
  | "PAUSA_GENERALE"
  | "RIPRENDI_GENERALE"
  | "BLACKOUT_ON"
  | "BLACKOUT_OFF"
  | "MOSTRA_RIQUADRO"
  | "NASCONDI_RIQUADRO"
  | "AGGIORNA_RIQUADRO"
  | "RICARICA_CONTENUTI"
  | "SCEGLI_CATEGORIA"
  | "MOSTRA_DOMANDA"
  | "PROSSIMA_DOMANDA"
  | "SELEZIONA"
  | "CONFERMA"
  | "CONFERMA_GIUSTA"
  | "CONFERMA_SBAGLIATA"
  | "RIVELA"
  | "PASSA_TURNO"
  | "ANNULLA"
  // gestito dal server, fuori dalle macchine a stati
  | "NUOVA_PARTITA";

export type StatoOpzione = "neutra" | "selezionata" | "giusta" | "sbagliata" | "soluzione";

export interface Modalita {
  id: number;
  nome: string;
}

export interface Riquadro {
  visibile: boolean;
  testo: string;
}

// ------------------------------------------------------------------ pubblico

export type StatoEstas = "SceltaCategoria" | "AttesaOggetto" | "OggettoInGioco" | "CategoriaFinita";

export interface EstasPubblico {
  stato: StatoEstas;
  categoria: string;
  oggetto: string;
  opzioni: { etichetta: string; stato: StatoOpzione }[];
  attesaEsitoMs: number; // config: per quanto un'opzione appena giudicata resta gialla
}

export interface VistaPubblico {
  blackout: boolean;
  riquadro: Riquadro;
  modalita: Modalita | null;
  estas: EstasPubblico | null;
}

// ------------------------------------------------------------------ plancia

export interface EventoAmmesso {
  evento: NomeEvento;
  arg: number;
}

export interface EstasPlancia {
  stato: StatoEstas;
  categorie: { nome: string; totale: number; giocati: number }[];
  categoriaCorrente: number;
  oggetto: string | null;
  oggettoAppartiene: boolean | null;
  anteprima: string | null;
  anteprimaAppartiene: boolean | null;
  statoDomanda: "" | "Mostrata" | "Selezionata" | "Giusta" | "Sbagliata" | "Rivelata";
  domandaConclusa: boolean;
  opzioni: { etichetta: string; stato: StatoOpzione; corretta: boolean }[];
  concorrenteDiTurno: number;
  numeroConcorrenti: number;
  attesaEsitoMs: number; // config: per quanto la plancia blocca "prossimo" dopo un giudizio
}

export interface EventoRegistrato {
  seq: number;
  evento: NomeEvento;
  arg: number;
  testo: string;
  t: number;
}

export interface EventoRifiutato {
  evento: NomeEvento;
  arg: number;
  testo: string;
  t: number;
  motivo: string;
}

export interface Transizione {
  macchina: string;
  da: string;
  evento: NomeEvento;
  a: string;
}

export interface VistaPlancia {
  statoSerata: "Lobby" | "SceltaModalita" | "InGioco" | "Chiusa";
  pausa: boolean;
  blackout: boolean;
  riquadro: Riquadro;
  numeroConcorrenti: number;
  contenuti: { estasCategorie: number; estasOggetti: number };
  modalita: (Modalita & { disponibile: boolean; giocata: boolean })[];
  modalitaAttiva: Modalita | null;
  statoModalita: string;
  estas: EstasPlancia | null;
  ammessi: EventoAmmesso[];
  debug: {
    lunghezzaRegistro: number;
    ultimiEventi: EventoRegistrato[];
    ultimiRifiutati: EventoRifiutato[];
    ultimeTransizioni: Transizione[];
  };
  server: {
    registro: string;
    avvisi: string[];
    nuovaPartitaAmmessa: boolean;
    pinAttivo: boolean;
  };
}

// ------------------------------------------------------------------ messaggi

export type Ruolo = "plancia" | "pubblico";
export type VistaDi<R extends Ruolo> = R extends "plancia" ? VistaPlancia : VistaPubblico;

export type DalClient =
  | { tipo: "hello"; ruolo: Ruolo; pin?: string }
  | { tipo: "event"; id: string; evento: NomeEvento; arg: number; testo: string }
  | { tipo: "ping" };

export type DalServer<V> =
  | { tipo: "snapshot" | "update"; seq: number; oraServer: number; vista: V }
  | { tipo: "ack"; id: string }
  | { tipo: "reject"; id: string | null; motivo: string }
  | { tipo: "pong"; oraServer: number };

// Vero se il server accetterebbe adesso quell'evento con quell'argomento.
export function ammesso(v: VistaPlancia, evento: NomeEvento, arg = -1): boolean {
  if (evento === "NUOVA_PARTITA") return v.server.nuovaPartitaAmmessa;
  return v.ammessi.some((a) => a.evento === evento && a.arg === arg);
}
