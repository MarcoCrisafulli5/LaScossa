#include "lascossa/esa.hpp"

#include <string>
#include <utility>

namespace lascossa {

namespace {
constexpr std::string_view kMacchina = "Esa";

Esito non_ammesso(TipoEvento ev, Esa::Stato s) {
    return Esito::rifiuto(std::string(nome(ev)) + " non ammesso in " + std::string(Esa::nome(s)));
}
}  // namespace

Esa::Esa(std::vector<FraseEsa> frasi, std::int64_t lettura_indizio_ms, std::int64_t timer_risposta_ms)
    : frasi_(std::move(frasi)),
      lettura_indizio_ms_(lettura_indizio_ms),
      timer_risposta_ms_(timer_risposta_ms),
      stato_(frasi_.empty() ? Stato::Finita : Stato::AttesaFrase) {}

std::string_view Esa::nome(Stato s) {
    switch (s) {
        case Stato::AttesaFrase: return "AttesaFrase";
        case Stato::InGioco: return "InGioco";
        case Stato::Indovinata: return "Indovinata";
        case Stato::Rivelata: return "Rivelata";
        case Stato::Scaduta: return "Scaduta";
        case Stato::Finita: return "Finita";
    }
    return "?";
}

const FraseEsa* Esa::frase_corrente() const {
    switch (stato_) {
        case Stato::InGioco:
        case Stato::Indovinata:
        case Stato::Rivelata:
        case Stato::Scaduta: return &frasi_[corrente_];
        case Stato::AttesaFrase:
        case Stato::Finita: return nullptr;
    }
    return nullptr;
}

const FraseEsa* Esa::prossima_frase() const {
    return stato_ == Stato::AttesaFrase && prossima_ < frasi_.size() ? &frasi_[prossima_] : nullptr;
}

bool Esa::frase_rivelata() const {
    return stato_ == Stato::Indovinata || stato_ == Stato::Rivelata || stato_ == Stato::Scaduta;
}

void Esa::vai(Stato nuovo, TipoEvento ev, Traccia* t) {
    traccia(t, kMacchina, nome(stato_), ev, nome(nuovo));
    stato_ = nuovo;
}

Esito Esa::gestisci(const Evento& e, Traccia* t) {
    switch (stato_) {
        case Stato::AttesaFrase: return su_attesa_frase(e, t);
        case Stato::InGioco: return su_in_gioco(e, t);
        case Stato::Indovinata:
        case Stato::Rivelata:
        case Stato::Scaduta: return su_conclusa(e, t);
        case Stato::Finita: return su_finita(e);
    }
    return Esito::rifiuto("stato sconosciuto");
}

// AttesaFrase: il pubblico vede solo il titolo; la plancia vede già indizi e soluzione in arrivo.
Esito Esa::su_attesa_frase(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::MostraDomanda:
            corrente_ = prossima_++;
            // Gli indizi scorrono per primi (uno ogni lettura_indizio_ms), poi il conto alla rovescia.
            timer_.avvia(e.t_ms, timer_risposta_ms_, static_cast<std::int64_t>(kIndiziEsa) * lettura_indizio_ms_);
            vai(Stato::InGioco, e.tipo, t);
            return Esito::ok();
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
        case TipoEvento::ScegliCategoria:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Rivela:
        case TipoEvento::PassaTurno:
        case TipoEvento::TimerScaduto:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

// InGioco: indizi in sequenza e timer. Giudica il conduttore; una risposta sbagliata non chiude la
// frase (il concorrente riprova finché c'è tempo).
Esito Esa::su_in_gioco(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::ConfermaGiusta:
            timer_.ferma(e.t_ms);
            vai(Stato::Indovinata, e.tipo, t);
            return Esito::ok();
        case TipoEvento::Rivela:
            timer_.ferma(e.t_ms);
            vai(Stato::Rivelata, e.tipo, t);
            return Esito::ok();
        case TipoEvento::TimerScaduto:
            if (!timer_.scadi(e.t_ms)) {
                return Esito::rifiuto("il timer non è ancora scaduto");
            }
            vai(Stato::Scaduta, e.tipo, t);
            return Esito::ok();
        case TipoEvento::ConfermaSbagliata:
            return Esito::rifiuto("in ESA una risposta sbagliata non chiude la frase: si aspetta la giusta o lo scadere del tempo");
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
        case TipoEvento::ScegliCategoria:
        case TipoEvento::MostraDomanda:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::PassaTurno:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

// Indovinata, Rivelata, Scaduta: la frase è sul pubblico. Da Scaduta il conduttore può ancora dare
// la risposta per giusta, se è arrivata allo scadere (meglio di un Annulla che farebbe ripartire il timer).
Esito Esa::su_conclusa(const Evento& e, Traccia* t) {
    switch (e.tipo) {
        case TipoEvento::ProssimaDomanda:
            timer_ = TimerGioco{};
            vai(prossima_ < frasi_.size() ? Stato::AttesaFrase : Stato::Finita, e.tipo, t);
            return Esito::ok();
        case TipoEvento::ConfermaGiusta:
            if (stato_ != Stato::Scaduta) break;
            vai(Stato::Indovinata, e.tipo, t);
            return Esito::ok();
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
        case TipoEvento::ScegliCategoria:
        case TipoEvento::MostraDomanda:
        case TipoEvento::Seleziona:
        case TipoEvento::Conferma:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Rivela:
        case TipoEvento::PassaTurno:
        case TipoEvento::TimerScaduto:
        case TipoEvento::Annulla: break;
    }
    return non_ammesso(e.tipo, stato_);
}

Esito Esa::su_finita(const Evento& e) { return non_ammesso(e.tipo, stato_); }

}  // namespace lascossa
