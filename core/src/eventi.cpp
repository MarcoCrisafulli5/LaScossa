#include "lascossa/eventi.hpp"

namespace lascossa {

std::string_view nome(TipoEvento t) {
    switch (t) {
        case TipoEvento::AvviaSerata: return "AVVIA_SERATA";
        case TipoEvento::ScegliModalita: return "SCEGLI_MODALITA";
        case TipoEvento::FineModalita: return "FINE_MODALITA";
        case TipoEvento::ChiudiSerata: return "CHIUDI_SERATA";
        case TipoEvento::PausaGenerale: return "PAUSA_GENERALE";
        case TipoEvento::RiprendiGenerale: return "RIPRENDI_GENERALE";
        case TipoEvento::BlackoutOn: return "BLACKOUT_ON";
        case TipoEvento::BlackoutOff: return "BLACKOUT_OFF";
        case TipoEvento::MostraRiquadro: return "MOSTRA_RIQUADRO";
        case TipoEvento::NascondiRiquadro: return "NASCONDI_RIQUADRO";
        case TipoEvento::AggiornaRiquadro: return "AGGIORNA_RIQUADRO";
        case TipoEvento::RicaricaContenuti: return "RICARICA_CONTENUTI";
        case TipoEvento::ConfermaGiusta: return "CONFERMA_GIUSTA";
        case TipoEvento::ConfermaSbagliata: return "CONFERMA_SBAGLIATA";
        case TipoEvento::Rivela: return "RIVELA";
        case TipoEvento::ScegliCategoria: return "SCEGLI_CATEGORIA";
        case TipoEvento::MostraDomanda: return "MOSTRA_DOMANDA";
        case TipoEvento::ProssimaDomanda: return "PROSSIMA_DOMANDA";
        case TipoEvento::Seleziona: return "SELEZIONA";
        case TipoEvento::Conferma: return "CONFERMA";
        case TipoEvento::PassaTurno: return "PASSA_TURNO";
        case TipoEvento::TimerScaduto: return "TIMER_SCADUTO";
        case TipoEvento::Annulla: return "ANNULLA";
    }
    return "?";
}

std::string_view nome(IdModalita m) {
    switch (m) {
        case IdModalita::Estas: return "Estas o non Estas";
        case IdModalita::Esa: return "ESA";
        case IdModalita::Date: return "Date";
        case IdModalita::LaScossa: return "La Scossa";
        case IdModalita::Triello: return "Triello";
        case IdModalita::Centos: return "Centos";
        case IdModalita::Ghigliottina: return "Ghigliottina";
        case IdModalita::Duelli: return "Duelli";
    }
    return "?";
}

std::optional<TipoEvento> tipo_da_nome(std::string_view n) {
    for (TipoEvento t : kTuttiGliEventi) {
        if (nome(t) == n) {
            return t;
        }
    }
    return std::nullopt;
}

}  // namespace lascossa
