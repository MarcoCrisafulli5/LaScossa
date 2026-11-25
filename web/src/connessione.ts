// Connessione WebSocket al server: hello, viste, ping/pong ogni 5 s, riconnessione automatica.
// Il browser non ha logica di gioco: invia eventi con nome e mostra la vista che riceve.

import { useCallback, useEffect, useRef, useState } from "react";
import type { DalClient, DalServer, NomeEvento, Ruolo, VistaDi } from "./protocollo";

const INTERVALLO_PING_MS = 5000;
const TIMEOUT_PONG_MS = 12000;
const ATTESA_RICONNESSIONE_MS = 1000;

export type StatoConnessione = "in connessione" | "connesso" | "disconnesso";

export interface Rifiuto {
  motivo: string;
  quando: number;
}

export interface Connessione<V> {
  stato: StatoConnessione;
  vista: V | null;
  rifiuto: Rifiuto | null;
  pinRichiesto: boolean;
  invia: (evento: NomeEvento, arg?: number, testo?: string) => void;
  impostaPin: (pin: string) => void;
}

const CHIAVE_PIN = "lascossa.pin";

function nuovoId(): string {
  // crypto.randomUUID esiste solo in contesto sicuro (https o localhost): da un tablet in Wi-Fi no.
  if (typeof crypto !== "undefined" && "randomUUID" in crypto && window.isSecureContext) {
    return crypto.randomUUID();
  }
  return `${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}`;
}

function indirizzo(): string {
  const protocollo = location.protocol === "https:" ? "wss" : "ws";
  return `${protocollo}://${location.host}/ws`;
}

export function useConnessione<R extends Ruolo>(ruolo: R): Connessione<VistaDi<R>> {
  const [stato, setStato] = useState<StatoConnessione>("in connessione");
  const [vista, setVista] = useState<VistaDi<R> | null>(null);
  const [rifiuto, setRifiuto] = useState<Rifiuto | null>(null);
  const [pinRichiesto, setPinRichiesto] = useState(false);
  const [pin, setPin] = useState<string>(() => sessionStorage.getItem(CHIAVE_PIN) ?? "");
  const socket = useRef<WebSocket | null>(null);

  useEffect(() => {
    let chiuso = false;
    let ping: number | undefined;
    let riconnessione: number | undefined;
    let ultimoPong = Date.now();

    const manda = (ws: WebSocket, m: DalClient) => ws.send(JSON.stringify(m));

    const apri = () => {
      setStato("in connessione");
      const ws = new WebSocket(indirizzo());
      socket.current = ws;

      ws.onopen = () => {
        ultimoPong = Date.now();
        manda(ws, ruolo === "plancia" && pin ? { tipo: "hello", ruolo, pin } : { tipo: "hello", ruolo });
        ping = window.setInterval(() => {
          if (Date.now() - ultimoPong > TIMEOUT_PONG_MS) {
            ws.close();  // nessuna risposta: si riconnette
            return;
          }
          manda(ws, { tipo: "ping" });
        }, INTERVALLO_PING_MS);
      };

      ws.onmessage = (e: MessageEvent<string>) => {
        const m = JSON.parse(e.data) as DalServer<VistaDi<R>>;
        switch (m.tipo) {
          case "snapshot":
          case "update":
            setStato("connesso");
            setPinRichiesto(false);
            setVista(m.vista);
            break;
          case "pong":
            ultimoPong = Date.now();
            break;
          case "ack":
            break;
          case "reject":
            if (m.id === null && m.motivo.includes("PIN")) setPinRichiesto(true);
            setRifiuto({ motivo: m.motivo, quando: Date.now() });
            break;
        }
      };

      ws.onclose = () => {
        window.clearInterval(ping);
        socket.current = null;
        if (chiuso) return;
        setStato("disconnesso");
        riconnessione = window.setTimeout(apri, ATTESA_RICONNESSIONE_MS);
      };
    };

    apri();
    return () => {
      chiuso = true;
      window.clearInterval(ping);
      window.clearTimeout(riconnessione);
      socket.current?.close();
    };
  }, [ruolo, pin]);

  const invia = useCallback((evento: NomeEvento, arg = -1, testo = "") => {
    const ws = socket.current;
    if (!ws || ws.readyState !== WebSocket.OPEN) {
      setRifiuto({ motivo: "non connesso al server: evento non inviato", quando: Date.now() });
      return;
    }
    const m: DalClient = { tipo: "event", id: nuovoId(), evento, arg, testo };
    ws.send(JSON.stringify(m));
  }, []);

  const impostaPin = useCallback((nuovo: string) => {
    sessionStorage.setItem(CHIAVE_PIN, nuovo);
    setPin(nuovo);
  }, []);

  return { stato, vista, rifiuto, pinRichiesto, invia, impostaPin };
}
