#include "lascossa/estas.hpp"

#include <string>
#include <utility>

namespace lascossa {

namespace {
constexpr std::string_view kMacchina = "Estas";

Esito non_ammesso(TipoEvento ev, Estas::Stato s) {
    return Esito::rifiuto(std::string(nome(ev)) + " non ammesso in " + std::string(Estas::nome(s)));
}
}  // namespace

Estas::Estas(std::vector<CategoriaEstas> categorie, int numero_concorrenti)
    : categorie_(std::move(categorie)), prossimo_(categorie_.size(), 0), turni_(numero_concorrenti) {}

std::string_view Estas::nome(Stato s) {
    switch (s) {
        case Stato::SceltaCategoria: return "SceltaCategoria";
        case Stato::OggettoInGioco: return "OggettoInGioco";
        case Stato::CategoriaFinita: return "CategoriaFinita";
    }
    return "?";
}

const OggettoEstas* Estas::oggetto_corrente() const {
    if (stato_ != Stato::OggettoInGioco || !categoria_valida(categoria_)) {
        return nullptr;
    }
    return &categorie_[static_cast<std::size_t>(categoria_)].oggetti[oggetto_];
}

bool Estas::ha_oggetti(int i) const {
    const auto idx = static_cast<std::size_t>(i);
    return prossimo_[idx] < categorie_[idx].oggetti.size();
}

void Estas::vai(Stato nuovo, TipoEvento ev, Traccia* t) {
    traccia(t, kMacchina, nome(stato_), ev, nome(nuovo));
    stato_ = nuovo;
}

Esito Estas::apri_categoria(int indice, TipoEvento ev, Traccia* t) {
    if (!categoria_valida(indice)) {
        return Esito::rifiuto("categoria " + std::to_string(indice) + " inesistente");
    }
    if (!ha_oggetti(indice)) {
        return Esito::rifiuto("categoria " + std::to_string(indice) + " senza oggetti da giocare");
    }
    categoria_ = indice;
    return apri_oggetto(ev, t);
}

Esito Estas::apri_oggetto(TipoEvento ev, Traccia* t) {
    const auto cat = static_cast<std::size_t>(categoria_);
    oggetto_ = prossimo_[cat]++;
    const bool appartiene = categorie_[cat].oggetti[oggetto_].appartiene;
    domanda_.emplace(2, appartiene ? kEstas : kNonEstas, /*riprova_dopo_errore=*/false);
    vai(Stato::OggettoInGioco, ev, t);
    return Esito::ok();
}

Esito Estas::gestisci(const Evento& e, Traccia* t) {
    switch (stato_) {
        case Stato::SceltaCategoria: return su_scelta_categoria(e, t);
        case Stato::OggettoInGioco: return su_oggetto_in_gioco(e, t);
        case Stato::CategoriaFinita: return su_categoria_finita(e, t);
    }
    return Esito::rifiuto("stato sconosciuto");
}

Esito Estas::su_scelta_categoria(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::ScegliCategoria: return apri_categoria(e.arg, e.tipo, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

Esito Estas::su_oggetto_in_gioco(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::Seleziona:
        case TipoEvento::Rivela: return domanda_->gestisci(e, t);
        case TipoEvento::Conferma: {
            Esito r = domanda_->gestisci(e, t);
            // Ipotesi da confermare: dopo una risposta sbagliata il turno passa al concorrente successivo.
            if (r && domanda_->stato() == CicloDomanda::Stato::Sbagliata) {
                turni_.passa();
            }
            return r;
        }
        case TipoEvento::PassaTurno:
            turni_.passa();
            traccia(t, kMacchina, nome(stato_), e.tipo, nome(stato_));
            return Esito::ok();
        case TipoEvento::ProssimaDomanda:
            if (!domanda_->conclusa()) {
                return Esito::rifiuto("l'oggetto corrente non ha ancora un esito");
            }
            if (ha_oggetti(categoria_)) {
                return apri_oggetto(e.tipo, t);
            }
            domanda_.reset();
            vai(Stato::CategoriaFinita, e.tipo, t);
            return Esito::ok();
        case TipoEvento::ScegliCategoria:
            // Cambio categoria possibile a ogni domanda, ma solo a domanda conclusa.
            if (!domanda_->conclusa()) {
                return Esito::rifiuto("l'oggetto corrente non ha ancora un esito");
            }
            return apri_categoria(e.arg, e.tipo, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

Esito Estas::su_categoria_finita(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::ScegliCategoria: return apri_categoria(e.arg, e.tipo, t);
        case TipoEvento::AvviaSerata:
        case TipoEvento::ScegliModalita:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::Rivela:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

}  // namespace lascossa
