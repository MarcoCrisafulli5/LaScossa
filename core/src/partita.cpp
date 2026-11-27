#include "lascossa/partita.hpp"

#include <utility>

namespace lascossa {

Partita::Partita(Configurazione cfg, Contenuti contenuti)
    : cfg_(cfg), contenuti_(std::move(contenuti)), serata_(std::make_unique<Serata>(cfg_, contenuti_)) {}

Esito Partita::applica(const Evento& e) {
    if (e.tipo == TipoEvento::Annulla) {
        // Le pause automatiche (Annulla precedenti, ripresa dopo un crash) non contano: si toglie l'ultimo evento vero.
        auto fine = registro_.end();
        while (fine != registro_.begin() && (fine - 1)->tipo == TipoEvento::PausaGenerale &&
               (fine - 1)->arg == kPausaAutomatica) {
            --fine;
        }
        if (fine == registro_.begin()) {
            rifiutati_.push_back({e, "niente da annullare"});
            return Esito::rifiuto("niente da annullare");
        }
        const Evento annullato = *(fine - 1);
        rigioca(std::vector<Evento>(registro_.begin(), fine - 1));
        if (serata_->prossima_scadenza()) {
            const Evento pausa{TipoEvento::PausaGenerale, kPausaAutomatica, annullato.t_ms};
            if (serata_->gestisci(pausa, &traccia_)) registro_.push_back(pausa);
        }
        return Esito::ok();
    }

    Esito r = serata_->gestisci(e, &traccia_);
    if (r) {
        registro_.push_back(e);
    } else {
        rifiutati_.push_back({e, r.motivo});
    }
    return r;
}

void Partita::rigioca(const std::vector<Evento>& eventi) {
    serata_ = std::make_unique<Serata>(cfg_, contenuti_);
    registro_.clear();
    traccia_.clear();
    for (const Evento& e : eventi) {
        if (serata_->gestisci(e, &traccia_)) {
            registro_.push_back(e);
        }
        // Un evento del registro che ora viene rifiutato indica un registro incoerente:
        // lo scartiamo e lo segnaliamo tra i rifiutati.
        else {
            rifiutati_.push_back({e, "rifiutato durante il replay"});
        }
    }
}

}  // namespace lascossa
