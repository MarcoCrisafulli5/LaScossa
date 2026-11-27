// La Partita è il punto d'ingresso: riceve gli eventi, li passa alla serata,
// tiene il registro degli eventi accettati e realizza "Annulla" rigiocando il registro.
#pragma once

#include <memory>
#include <vector>

#include "lascossa/esito.hpp"
#include "lascossa/serata.hpp"

namespace lascossa {

// Argomento di una PAUSA_GENERALE messa dal sistema (dopo un Annulla o la ripresa da un crash con un
// timer che correva), non dal conduttore: un Annulla successivo la salta.
inline constexpr int kPausaAutomatica = 1;

struct EventoRifiutato {
    Evento evento;
    std::string motivo;
};

class Partita {
public:
    Partita(Configurazione cfg, Contenuti contenuti);

    // Applica un evento. Annulla rigioca il registro senza l'ultimo evento accettato; se dopo il
    // replay un timer di gioco corre, la serata va in pausa generale con il timer fermo al valore
    // che aveva nel momento annullato (altrimenti potrebbe scadere subito per il tempo passato).
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
