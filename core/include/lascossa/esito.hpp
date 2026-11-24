// Esito di un evento e traccia delle transizioni (per registro e debug).
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "lascossa/eventi.hpp"

namespace lascossa {

struct Esito {
    bool accettato = false;
    std::string motivo;  // vuoto se accettato

    static Esito ok() { return {true, {}}; }
    static Esito rifiuto(std::string m) { return {false, std::move(m)}; }
    explicit operator bool() const { return accettato; }
};

// Una transizione eseguita: macchina, stato di partenza, evento, stato di arrivo.
struct Transizione {
    std::string_view macchina;
    std::string_view da;
    TipoEvento evento;
    std::string_view a;
};

using Traccia = std::vector<Transizione>;

// Registra una transizione se la traccia è attiva (puntatore non nullo).
inline void traccia(Traccia* t, std::string_view macchina, std::string_view da, TipoEvento ev,
                    std::string_view a) {
    if (t != nullptr) {
        t->push_back({macchina, da, ev, a});
    }
}

}  // namespace lascossa
