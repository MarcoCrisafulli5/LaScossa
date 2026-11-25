// Serializzazione JSON delle viste per il protocollo WebSocket.
// I tipi TypeScript corrispondenti sono in web/src/protocollo.ts: vanno tenuti allineati a mano
// finché non c'è il generatore dallo schema JSON (vedi documento di architettura).
#pragma once

#include <nlohmann/json.hpp>

#include "lascossa/viste.hpp"

namespace lascossa::server {

nlohmann::json a_json(const VistaPlancia& v);
nlohmann::json a_json(const VistaPubblico& v);

}  // namespace lascossa::server
