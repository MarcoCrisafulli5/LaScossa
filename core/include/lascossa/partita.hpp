// La Partita è il punto d'ingresso: riceve gli eventi, li passa alla serata,
// tiene il registro degli eventi accettati e realizza "Annulla" rigiocando il registro.
#pragma once

#include <memory>
#include <vector>

#include "lascossa/esito.hpp"
#include "lascossa/serata.hpp"

namespace lascossa {

struct EventoRifiutato {
    Evento evento;
    std::string motivo;
};

class Partita {
public:
    Partita(Configurazione cfg, Contenuti contenuti);

    // Applica un evento. Annulla rigioca il registro senza l'ultimo evento accettato.
    Esito applica(const Evento& e);

    const Serata& serata() const { return *serata_; }
    const std::vector<Evento>& registro() const { return registro_; }
    const Traccia& traccia() const { return traccia_; }
    const std::vector<EventoRifiutato>& rifiutati() const { return rifiutati_; }

    // Ricostruisce lo stato da zero rigiocando gli eventi dati (ripresa dopo un crash).
    void rigioca(const std::vector<Evento>& eventi);

private:
    Configurazione cfg_;
    Contenuti contenuti_;
    std::unique_ptr<Serata> serata_;
    std::vector<Evento> registro_;
    Traccia traccia_;
    std::vector<EventoRifiutato> rifiutati_;
};

}  // namespace lascossa
