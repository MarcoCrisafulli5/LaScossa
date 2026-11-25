---
paths:
  - "web/**"
---

# Convenzioni dell'interfaccia (React + Vite + TypeScript)

- Nessuna libreria oltre a React: niente router (la pagina si sceglie da `location.pathname` in
  `main.tsx`), niente state manager, WebSocket nativo del browser.
- **Nessuna logica di gioco nel browser.** Un pulsante di evento è abilitato solo se
  `ammesso(vista, evento, arg)` è vero; il browser non deduce le regole dallo stato.
- `web/src/protocollo.ts` è lo specchio a mano di `server/src/json_viste.cpp` e `regia.cpp`.
  Se cambi un campo, cambialo da entrambe le parti nello stesso intervento.
- TypeScript `strict` con `noUncheckedIndexedAccess`. `npm run build` esegue `tsc --noEmit`: deve passare.
- **Lockfile e Docker**: npm su Windows omette dal `package-lock.json` le dipendenze dei binding
  opzionali di altre piattaforme (es. `@emnapi/*` di `@rolldown/binding-wasm32-wasi`) e poi `npm ci`
  nel Dockerfile fallisce con "Missing: … from lock file". Dopo ogni `npm install` su Windows,
  rigenera il lockfile in Linux:
  `docker run --rm -v "<repo>/web/package.json:/w/package.json:ro" -v "<repo>/web/package-lock.json:/w/package-lock.json" -w /w node:24-alpine npm install --package-lock-only --ignore-scripts`
  (in Git Bash anteponi `MSYS_NO_PATHCONV=1`).
- Testi in italiano. Componenti in `PascalCase.tsx`, una cartella per pagina (`plancia/`, `pubblico/`, `debug/`).

## Plancia (requisiti non funzionali)

- Un'azione = un pulsante grande e ben etichettato. Nessuna finestra di conferma durante il gioco:
  gli errori si correggono con "Annulla ultima azione".
- Pulsanti "segna" (conferma, passa turno) separati da quelli "mostra" (rivela, prossimo).
- I rifiuti del server compaiono come messaggio temporaneo con il motivo.

## Pubblico

- Palco 16:9 centrato; tutte le misure in unità `cqw`/`cqh` del palco, così scala su ogni proiettore.
- Testi grandi e ad alto contrasto. Mai messaggi di errore o di connessione davanti alla sala.
- Colori, logo e font solo come variabili CSS in `stile.css` (grafica segnaposto, sostituibile).
  Non riprodurre marchi o grafiche de L'Eredità.
