import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// In sviluppo (npm run dev) Vite serve l'interfaccia su :5173 e inoltra il WebSocket
// al server C++ su :8080. In produzione il server C++ serve direttamente web/dist.
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      "/ws": { target: "ws://localhost:8080", ws: true },
    },
  },
});
