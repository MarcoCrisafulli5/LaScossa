#include "lascossa/ciclo_domanda.hpp"

#include <string>

namespace lascossa {

namespace {
constexpr std::string_view kMacchina = "Domanda";

Esito non_ammesso(TipoEvento ev, CicloDomanda::Stato s) {
    return Esito::rifiuto(std::string(nome(ev)) + " non ammesso in " + std::string(CicloDomanda::nome(s)));
}
}  // namespace

CicloDomanda::CicloDomanda(int num_opzioni, int corretta, bool riprova_dopo_errore)
    : corretta_(corretta),
      riprova_(riprova_dopo_errore),
      bruciate_(static_cast<std::size_t>(num_opzioni > 0 ? num_opzioni : 0), false) {}

std::string_view CicloDomanda::nome(Stato s) {
    switch (s) {
        case Stato::Mostrata: return "Mostrata";
        case Stato::Selezionata: return "Selezionata";
        case Stato::Giusta: return "Giusta";
        case Stato::Sbagliata: return "Sbagliata";
        case Stato::Rivelata: return "Rivelata";
    }
    return "?";
}

void CicloDomanda::vai(Stato nuovo, TipoEvento ev, Traccia* t) {
    traccia(t, kMacchina, nome(stato_), ev, nome(nuovo));
    stato_ = nuovo;
}

Esito CicloDomanda::gestisci(const Evento& e, Traccia* t) {
    switch (stato_) {
        case Stato::Mostrata: return su_mostrata(e, t);
        case Stato::Selezionata: return su_selezionata(e, t);
        case Stato::Giusta: return su_concluso(e);
        case Stato::Sbagliata: return su_sbagliata(e, t);
        case Stato::Rivelata: return su_concluso(e);
    }
    return Esito::rifiuto("stato sconosciuto");
}

Esito CicloDomanda::seleziona(int i, Traccia* t, TipoEvento ev) {
    if (!valida(i)) {
        return Esito::rifiuto("opzione " + std::to_string(i) + " inesistente");
    }
    if (bruciate_[static_cast<std::size_t>(i)]) {
        return Esito::rifiuto("opzione " + std::to_string(i) + " già sbagliata");
    }
    selezionata_ = i;
    vai(Stato::Selezionata, ev, t);
    return Esito::ok();
}

// Mostrata: si aspetta la risposta del concorrente.
Esito CicloDomanda::su_mostrata(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::Seleziona: return seleziona(e.arg, t, e.tipo);
        case TipoEvento::Rivela: return rivela(e.tipo, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::MostraDomanda:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::TimerScaduto:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

// Selezionata (giallo): il conduttore può cambiare selezione (clic sbagliato) o confermarla.
Esito CicloDomanda::su_selezionata(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::Seleziona: return seleziona(e.arg, t, e.tipo);
        case TipoEvento::Rivela: return rivela(e.tipo, t);
        case TipoEvento::Conferma:
            if (selezionata_ == corretta_) {
                vai(Stato::Giusta, e.tipo, t);
            } else {
                bruciate_[static_cast<std::size_t>(selezionata_)] = true;
                vai(Stato::Sbagliata, e.tipo, t);
            }
            return Esito::ok();
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::MostraDomanda:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::PassaTurno:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::TimerScaduto:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

// Giusta (verde) e Rivelata: stati finali della domanda.
Esito CicloDomanda::su_concluso(const Evento& e) { return non_ammesso(e.tipo, stato_); }

// Rivela: il conduttore mostra la soluzione (nessuno ha risposto o lo decide lui).
Esito CicloDomanda::rivela(TipoEvento ev, Traccia* t) {
    vai(Stato::Rivelata, ev, t);
    return Esito::ok();
}

// Sbagliata (rosso): se la domanda ammette altri tentativi si può selezionare un'altra opzione.
Esito CicloDomanda::su_sbagliata(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::Seleziona:
            if (!riprova_) {
                return Esito::rifiuto("domanda conclusa: non ammette altri tentativi");
            }
            return seleziona(e.arg, t, e.tipo);
        case TipoEvento::Rivela:
            if (!riprova_) {
                return Esito::rifiuto("domanda conclusa: soluzione già nota");
            }
            return rivela(e.tipo, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::ScegliCategoria:
        case TipoEvento::MostraDomanda:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::TimerScaduto:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

}  // namespace lascossa
