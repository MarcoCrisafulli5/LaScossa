import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./stile.css";
import { Plancia } from "./plancia/Plancia";
import { Pubblico } from "./pubblico/Pubblico";
import { Debug } from "./debug/Debug";

// Tre pagine dello stesso server, scelte dall'indirizzo: /plancia (default), /pubblico, /debug.
function Pagina() {
  const percorso = location.pathname;
  if (percorso.startsWith("/pubblico")) return <Pubblico />;
  if (percorso.startsWith("/debug")) return <Debug />;
  return <Plancia />;
}

const radice = document.getElementById("app");
if (!radice) throw new Error("manca l'elemento #app");
createRoot(radice).render(
  <StrictMode>
    <Pagina />
  </StrictMode>,
);
