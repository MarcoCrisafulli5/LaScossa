#include "lascossa/serata.hpp"

#include <string>
#include <utility>

namespace lascossa {

namespace {
constexpr std::string_view kMacchina = "Serata";

Esito non_ammesso(TipoEvento ev, Serata::Stato s) {
    return Esito::rifiuto(std::string(nome(ev)) + " non ammesso in " + std::string(Serata::nome(s)));
}
}  // namespace

Serata::Serata(Configurazione cfg, Contenuti contenuti) : cfg_(cfg), contenuti_(std::move(contenuti)) {}

std::string_view Serata::nome(Stato s) {
    switch (s) {
        case Stato::Lobby: return "Lobby";
        case Stato::SceltaModalita: return "SceltaModalita";
        case Stato::InGioco: return "InGioco";
        case Stato::Chiusa: return "Chiusa";
    }
    return "?";
}

void Serata::vai(Stato nuovo, TipoEvento ev, Traccia* t) {
    traccia(t, kMacchina, nome(stato_), ev, nome(nuovo));
    stato_ = nuovo;
}

std::unique_ptr<Modalita> Serata::crea_modalita(int id) const {
    if (id < 0 || id > static_cast<int>(IdModalita::Duelli)) {
        return nullptr;
    }
    switch (static_cast<IdModalita>(id)) {
        case IdModalita::Estas:
            return std::make_unique<Estas>(contenuti_.estas, cfg_.numero_concorrenti);
        case IdModalita::Esa:
        case IdModalita::Date:
        case IdModalita::LaScossa:
        case IdModalita::Triello:
        case IdModalita::Centos:
        case IdModalita::Ghigliottina:
        case IdModalita::Duelli: return nullptr;  // non ancora implementate
    }
    return nullptr;
}

Esito Serata::gestisci(const Evento& e, Traccia* t) {
    switch (stato_) {
        case Stato::Lobby: return su_lobby(e, t);
        case Stato::SceltaModalita: return su_scelta_modalita(e, t);
        case Stato::InGioco: return su_in_gioco(e, t);
        case Stato::Chiusa: return su_chiusa(e);
    }
    return Esito::rifiuto("stato sconosciuto");
}

// Eventi validi in ogni stato aperto: pausa generale e blackout. Non cambiano lo stato.
Esito Serata::globale(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::PausaGenerale:
            if (pausa_) return Esito::rifiuto("già in pausa");
            pausa_ = true;
            break;
        case TipoEvento::RiprendiGenerale:
            if (!pausa_) return Esito::rifiuto("non in pausa");
            pausa_ = false;
            break;
        case TipoEvento::BlackoutOn:
            if (blackout_) return Esito::rifiuto("blackout già attivo");
            blackout_ = true;
            break;
        case TipoEvento::BlackoutOff:
            if (!blackout_) return Esito::rifiuto("blackout non attivo");
            blackout_ = false;
            break;
        case TipoEvento::MostraRiquadro:
            if (riquadro_visibile_) return Esito::rifiuto("riquadro già visibile");
            riquadro_visibile_ = true;
            break;
        case TipoEvento::NascondiRiquadro:
            if (!riquadro_visibile_) return Esito::rifiuto("riquadro già nascosto");
            riquadro_visibile_ = false;
            break;
        case TipoEvento::AggiornaRiquadro:
            riquadro_testo_ = e.testo;
            break;
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: return non_ammesso(e.tipo, stato_);
    }
    traccia(t, kMacchina, nome(stato_), e.tipo, nome(stato_));
    return Esito::ok();
}

Esito Serata::su_lobby(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::AvviaSerata: vai(Stato::SceltaModalita, e.tipo, t); return Esito::ok();
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro: return globale(e, t);
        case TipoEvento::RicaricaContenuti:
            // il server rilegge i CSV fuori dalla macchina; qui si registra solo l'evento
            traccia(t, kMacchina, nome(stato_), e.tipo, nome(stato_));
            return Esito::ok();
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

Esito Serata::su_scelta_modalita(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::ScegliModalita: {
            auto m = crea_modalita(e.arg);
            if (!m) {
                return Esito::rifiuto("modalità " + std::to_string(e.arg) + " non disponibile");
            }
            giocate_.push_back(m->id());
            modalita_ = std::move(m);
            vai(Stato::InGioco, e.tipo, t);
            return Esito::ok();
        }
        case TipoEvento::ChiudiSerata: vai(Stato::Chiusa, e.tipo, t); return Esito::ok();
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro: return globale(e, t);
        case TipoEvento::RicaricaContenuti:
            // il server rilegge i CSV fuori dalla macchina; qui si registra solo l'evento
            traccia(t, kMacchina, nome(stato_), e.tipo, nome(stato_));
            return Esito::ok();
        case TipoEvento::AvviaSerata:
        case TipoEvento::FineModalita:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

// InGioco: la serata gestisce fine modalità ed eventi globali, il resto scende alla modalità attiva.
Esito Serata::su_in_gioco(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::FineModalita:
            modalita_.reset();
            vai(Stato::SceltaModalita, e.tipo, t);
            return Esito::ok();
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro: return globale(e, t);
        case TipoEvento::ScegliCategoria:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
            if (pausa_) {
                return Esito::rifiuto("gioco in pausa generale");
            }
            return modalita_->gestisci(e, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

Esito Serata::su_chiusa(const Evento& e) { return non_ammesso(e.tipo, stato_); }

}  // namespace lascossa
