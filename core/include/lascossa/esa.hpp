// Modalità 2 - ESA.
// Tre indizi compaiono uno alla volta (uno ogni `lettura_indizio_ms`), poi parte il timer di
// risposta; il concorrente deve indovinare la frase. La frase si rivela alla risposta giusta,
// a comando del conduttore o allo scadere del tempo. Niente turni: se il tempo scade si passa
// alla frase successiva e a un altro concorrente, scelto dai conduttori.
//
// AttesaFrase -> (MOSTRA_DOMANDA) InGioco -> Indovinata | Rivelata | Scaduta
//             -> (PROSSIMA_DOMANDA) AttesaFrase ... -> Finita
//
// Il timer parte con MOSTRA_DOMANDA ma il conto alla rovescia comincia dopo i 3 indizi: indizi e
// timer usano lo stesso orologio, così la pausa generale ferma entrambi.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "lascossa/modalita.hpp"
#include "lascossa/timer.hpp"

namespace lascossa {

inline constexpr std::size_t kIndiziEsa = 3;

struct FraseEsa {
    std::array<std::string, kIndiziEsa> indizi;
    std::string soluzione;
};

class Esa final : public Modalita {
public:
    enum class Stato { AttesaFrase, InGioco, Indovinata, Rivelata, Scaduta, Finita };

    Esa(std::vector<FraseEsa> frasi, std::int64_t lettura_indizio_ms, std::int64_t timer_risposta_ms);

    IdModalita id() const override { return IdModalita::Esa; }
    Esito gestisci(const Evento& e, Traccia* t) override;
    std::string_view nome_stato() const override { return nome(stato_); }
    std::unique_ptr<Modalita> clona() const override { return std::make_unique<Esa>(*this); }

    void pausa_timer(std::int64_t ora) override { timer_.pausa(ora); }
    void riprendi_timer(std::int64_t ora) override { timer_.riprendi(ora); }
    std::optional<std::int64_t> prossima_scadenza() const override { return timer_.prossima_scadenza(); }

    Stato stato() const { return stato_; }
    // La frase in gioco o appena conclusa; nullptr in AttesaFrase e Finita.
    const FraseEsa* frase_corrente() const;
    // In AttesaFrase: la frase che MOSTRA_DOMANDA porterà in gioco (anteprima per la plancia).
    const FraseEsa* prossima_frase() const;
    // Vero quando la soluzione può andare sul pubblico (frase indovinata, rivelata o scaduta).
    bool frase_rivelata() const;
    std::size_t giocate() const { return prossima_; }  // frasi già portate in gioco
    std::size_t totale() const { return frasi_.size(); }
    const TimerGioco& timer() const { return timer_; }
    std::int64_t lettura_indizio_ms() const { return lettura_indizio_ms_; }

    static std::string_view nome(Stato s);

private:
    Esito su_attesa_frase(const Evento& e, Traccia* t);
    Esito su_in_gioco(const Evento& e, Traccia* t);
    Esito su_conclusa(const Evento& e, Traccia* t);
    Esito su_finita(const Evento& e);
    void vai(Stato nuovo, TipoEvento ev, Traccia* t);

    std::vector<FraseEsa> frasi_;
    std::int64_t lettura_indizio_ms_;
    std::int64_t timer_risposta_ms_;
    Stato stato_ = Stato::AttesaFrase;
    std::size_t prossima_ = 0;  // indice della prossima frase da giocare
    std::size_t corrente_ = 0;  // valido da InGioco in poi
    TimerGioco timer_;
};

}  // namespace lascossa
