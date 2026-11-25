// Interfaccia comune a tutte le modalità. La serata contiene la modalità attiva
// e le passa gli eventi che non gestisce lei stessa.
#pragma once

#include <memory>
#include <string_view>

#include "lascossa/esito.hpp"
#include "lascossa/eventi.hpp"

namespace lascossa {

class Modalita {
public:
    virtual ~Modalita() = default;
    virtual IdModalita id() const = 0;
    virtual Esito gestisci(const Evento& e, Traccia* t) = 0;
    virtual std::string_view nome_stato() const = 0;
    // Copia profonda: serve a provare un evento su una copia della serata
    // (eventi ammessi in plancia) senza toccare la partita vera.
    virtual std::unique_ptr<Modalita> clona() const = 0;

protected:
    Modalita() = default;
    Modalita(const Modalita&) = default;
    Modalita& operator=(const Modalita&) = default;
};

}  // namespace lascossa
