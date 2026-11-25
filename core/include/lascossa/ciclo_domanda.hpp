// Ciclo comune di una domanda a scelta: Mostrata -> Selezionata (giallo) -> Giusta (verde) / Sbagliata (rosso).
// Rivela porta a Rivelata (soluzione mostrata senza risposta).
// Riusato dalle modalità a scelta (Estas, Date, La Scossa, Triello).
#pragma once

#include <string_view>
#include <vector>

#include "lascossa/esito.hpp"
#include "lascossa/eventi.hpp"

namespace lascossa {

class CicloDomanda {
public:
    enum class Stato { Mostrata, Selezionata, Giusta, Sbagliata, Rivelata };

    // num_opzioni: quante risposte possibili; corretta: indice della risposta giusta.
    // riprova_dopo_errore: se true, dopo una risposta sbagliata la domanda resta aperta
    // (l'opzione sbagliata resta rossa e non si può più scegliere), come nella modalità Date.
    CicloDomanda(int num_opzioni, int corretta, bool riprova_dopo_errore);

    Esito gestisci(const Evento& e, Traccia* t);

    Stato stato() const { return stato_; }
    int selezionata() const { return selezionata_; }
    int corretta() const { return corretta_; }
    int num_opzioni() const { return static_cast<int>(bruciate_.size()); }
    bool bruciata(int i) const { return valida(i) && bruciate_[static_cast<std::size_t>(i)]; }
    bool conclusa() const {
        return stato_ == Stato::Giusta || stato_ == Stato::Rivelata || (stato_ == Stato::Sbagliata && !riprova_);
    }

    static std::string_view nome(Stato s);

private:
    Esito su_mostrata(const Evento& e, Traccia* t);
    Esito su_selezionata(const Evento& e, Traccia* t);
    Esito su_concluso(const Evento& e);
    Esito su_sbagliata(const Evento& e, Traccia* t);

    Esito seleziona(int i, Traccia* t, TipoEvento ev);
    Esito rivela(TipoEvento ev, Traccia* t);
    void vai(Stato nuovo, TipoEvento ev, Traccia* t);
    bool valida(int i) const { return i >= 0 && i < static_cast<int>(bruciate_.size()); }

    Stato stato_ = Stato::Mostrata;
    int corretta_;
    bool riprova_;
    int selezionata_ = -1;
    std::vector<bool> bruciate_;
};

}  // namespace lascossa
