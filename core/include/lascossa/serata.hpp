// Macchina a stati della serata: livello più alto.
// Lobby -> SceltaModalita -> InGioco(modalità) -> SceltaModalita -> ... -> Chiusa
// Pausa generale, blackout e riquadro punteggio valgono in ogni stato tranne Chiusa.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "lascossa/esito.hpp"
#include "lascossa/estas.hpp"
#include "lascossa/modalita.hpp"

namespace lascossa {

struct Configurazione {
    int numero_concorrenti = 4;
};

struct Contenuti {
    std::vector<CategoriaEstas> estas;
    // le altre modalità si aggiungeranno qui
};

class Serata {
public:
    enum class Stato { Lobby, SceltaModalita, InGioco, Chiusa };

    Serata(Configurazione cfg, Contenuti contenuti);

    Esito gestisci(const Evento& e, Traccia* t);

    Stato stato() const { return stato_; }
    bool in_pausa() const { return pausa_; }
    bool blackout() const { return blackout_; }
    bool riquadro_visibile() const { return riquadro_visibile_; }
    const std::string& riquadro_testo() const { return riquadro_testo_; }
    const Modalita* modalita() const { return modalita_.get(); }
    const std::vector<IdModalita>& giocate() const { return giocate_; }

    static std::string_view nome(Stato s);

private:
    Esito su_lobby(const Evento& e, Traccia* t);
    Esito su_scelta_modalita(const Evento& e, Traccia* t);
    Esito su_in_gioco(const Evento& e, Traccia* t);
    Esito su_chiusa(const Evento& e);
    Esito globale(const Evento& e, Traccia* t);  // pausa e blackout

    std::unique_ptr<Modalita> crea_modalita(int id) const;
    void vai(Stato nuovo, TipoEvento ev, Traccia* t);

    Configurazione cfg_;
    Contenuti contenuti_;
    Stato stato_ = Stato::Lobby;
    bool pausa_ = false;
    bool blackout_ = false;
    bool riquadro_visibile_ = false;
    std::string riquadro_testo_;
    std::unique_ptr<Modalita> modalita_;
    std::vector<IdModalita> giocate_;
};

}  // namespace lascossa
