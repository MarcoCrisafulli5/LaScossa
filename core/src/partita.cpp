#include "lascossa/partita.hpp"

#include <utility>

namespace lascossa {

Partita::Partita(Configurazione cfg, Contenuti contenuti)
    : cfg_(cfg), contenuti_(std::move(contenuti)), serata_(std::make_unique<Serata>(cfg_, contenuti_)) {}

Esito Partita::applica(const Evento& e) {
    if (e.tipo == TipoEvento::Annulla) {
        if (registro_.empty()) {
            rifiutati_.push_back({e, "niente da annullare"});
            return Esito::rifiuto("niente da annullare");
        }
        std::vector<Evento> eventi(registro_.begin(), registro_.end() - 1);
        rigioca(eventi);
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
