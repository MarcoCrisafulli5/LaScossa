// Timer di gioco (RC-05): un conto alla rovescia che governa la partita (la sua scadenza è un
// evento TIMER_SCADUTO registrato). Vive nella modalità, è copiabile e non legge mai l'orologio:
// ogni operazione riceve l'ora dall'evento (Evento::t_ms), così il replay del registro è identico.
//
// Un timer può partire in ritardo (es. ESA: prima scorrono gli indizi, poi il conto alla rovescia).
// "Totale" = ritardo + durata: è la parte di orologio che i browser usano anche per gli indizi.
// In pausa si conserva il tempo che mancava alla scadenza, ritardo compreso.
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>

namespace lascossa {

class TimerGioco {
public:
    enum class Stato { Spento, Corre, InPausa, Fermo, Scaduto };

    // Avvia: il conto alla rovescia di `durata_ms` comincia `ritardo_ms` dopo `ora`.
    void avvia(std::int64_t ora, std::int64_t durata_ms, std::int64_t ritardo_ms = 0) {
        durata_ = durata_ms;
        totale_ = ritardo_ms + durata_ms;
        scadenza_ = ora + totale_;
        residuo_ = totale_;
        stato_ = Stato::Corre;
    }

    // Pausa generale: congela il tempo che manca alla scadenza.
    void pausa(std::int64_t ora) {
        if (stato_ != Stato::Corre) return;
        residuo_ = std::max<std::int64_t>(0, scadenza_ - ora);
        stato_ = Stato::InPausa;
    }

    void riprendi(std::int64_t ora) {
        if (stato_ != Stato::InPausa) return;
        scadenza_ = ora + residuo_;
        stato_ = Stato::Corre;
    }

    // Esito deciso prima della scadenza (risposta giusta, rivela): il timer si ferma dov'era.
    void ferma(std::int64_t ora) {
        if (stato_ == Stato::Corre) residuo_ = std::max<std::int64_t>(0, scadenza_ - ora);
        if (stato_ == Stato::Corre || stato_ == Stato::InPausa) stato_ = Stato::Fermo;
    }

    // La scadenza è arrivata (evento TIMER_SCADUTO): vero se il timer correva ed era davvero scaduto.
    bool scadi(std::int64_t ora) {
        if (stato_ != Stato::Corre || ora < scadenza_) return false;
        residuo_ = 0;
        stato_ = Stato::Scaduto;
        return true;
    }

    // Ora di scadenza se il timer corre (il server ci programma TIMER_SCADUTO), altrimenti nullopt.
    std::optional<std::int64_t> prossima_scadenza() const {
        return stato_ == Stato::Corre ? std::optional<std::int64_t>(scadenza_) : std::nullopt;
    }

    Stato stato() const { return stato_; }
    std::int64_t durata_ms() const { return durata_; }
    std::int64_t totale_ms() const { return totale_; }
    std::int64_t scadenza_ms() const { return scadenza_; }  // valida solo se Corre
    std::int64_t residuo_ms() const { return residuo_; }    // tempo che mancava, se non Corre

    static std::string_view nome(Stato s) {
        switch (s) {
            case Stato::Spento: return "spento";
            case Stato::Corre: return "corre";
            case Stato::InPausa: return "pausa";
            case Stato::Fermo: return "fermo";
            case Stato::Scaduto: return "scaduto";
        }
        return "?";
    }

private:
    Stato stato_ = Stato::Spento;
    std::int64_t durata_ = 0;
    std::int64_t totale_ = 0;
    std::int64_t scadenza_ = 0;
    std::int64_t residuo_ = 0;
};

}  // namespace lascossa
