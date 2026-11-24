// Eventi del gioco. Ogni cambiamento di stato nasce da un Evento.
// Nota: in futuro questo file sarà generato dallo schema JSON (schema/).
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace lascossa {

enum class TipoEvento : std::uint8_t {
    // --- livello serata ---
    AvviaSerata,       // Lobby -> SceltaModalita
    ScegliModalita,    // arg = IdModalita
    FineModalita,      // InGioco -> SceltaModalita
    ChiudiSerata,      // SceltaModalita -> Chiusa
    PausaGenerale,     // congela tutti i timer (globale)
    RiprendiGenerale,
    BlackoutOn,        // schermo pubblico nero/logo (globale)
    BlackoutOff,
    MostraRiquadro,    // riquadro punteggio visibile sullo schermo pubblico (globale)
    NascondiRiquadro,
    AggiornaRiquadro,  // testo = nuovo contenuto del riquadro
    RicaricaContenuti, // solo fuori dal gioco: il server rilegge i CSV
    // --- livello modalità / domanda ---
    ScegliCategoria,   // arg = indice categoria (o round)
    ProssimaDomanda,
    Seleziona,         // arg = indice opzione scelta dal concorrente
    Conferma,          // modalità a scelta: la macchina confronta con la soluzione -> verde o rosso
    ConfermaGiusta,    // modalità a risposta aperta (ESA, Centos, Duelli): giudica il conduttore
    ConfermaSbagliata,
    Rivela,            // mostra la soluzione senza risposta del concorrente
    PassaTurno,
    // --- gestito dalla Partita, non dalle macchine ---
    Annulla,
};

enum class IdModalita : std::uint8_t {
    Estas,
    Esa,
    Date,
    LaScossa,
    Triello,
    Centos,
    Ghigliottina,
    Duelli,
};

struct Evento {
    TipoEvento tipo;
    int arg = -1;           // significato dipende dal tipo
    std::int64_t t_ms = 0;  // ora dell'evento, assegnata dal server (le macchine non leggono l'orologio)
    std::string testo{};    // usato da AggiornaRiquadro
};

std::string_view nome(TipoEvento t);
std::string_view nome(IdModalita m);

}  // namespace lascossa
