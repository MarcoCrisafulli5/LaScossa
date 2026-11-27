// Modalità 1 - Estas o non Estas.
// Una categoria (es. "Piatti tipici regionali"), una serie di oggetti: il concorrente dice
// se l'oggetto appartiene (Estas, opzione 0) o no (Non Estas, opzione 1).
//
// SceltaCategoria -> AttesaOggetto -> (MOSTRA_DOMANDA) OggettoInGioco -> (PROSSIMA_DOMANDA)
// AttesaOggetto ... -> CategoriaFinita. In AttesaOggetto il pubblico vede solo la categoria,
// così i conduttori possono soffermarsi su regole e dubbi prima di rivelare l'oggetto.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lascossa/ciclo_domanda.hpp"
#include "lascossa/modalita.hpp"
#include "lascossa/turni.hpp"

namespace lascossa {

struct OggettoEstas {
    std::string nome;
    bool appartiene;
};

struct CategoriaEstas {
    std::string nome;
    std::vector<OggettoEstas> oggetti;
};

class Estas final : public Modalita {
public:
    enum class Stato { SceltaCategoria, AttesaOggetto, OggettoInGioco, CategoriaFinita };

    static constexpr int kEstas = 0;
    static constexpr int kNonEstas = 1;

    Estas(std::vector<CategoriaEstas> categorie, int numero_concorrenti);

    IdModalita id() const override { return IdModalita::Estas; }
    Esito gestisci(const Evento& e, Traccia* t) override;
    std::string_view nome_stato() const override { return nome(stato_); }
    std::unique_ptr<Modalita> clona() const override { return std::make_unique<Estas>(*this); }

    Stato stato() const { return stato_; }
    int categoria() const { return categoria_; }
    const OggettoEstas* oggetto_corrente() const;
    // In AttesaOggetto: l'oggetto che MOSTRA_DOMANDA porterà in gioco (anteprima per la plancia).
    const OggettoEstas* prossimo_oggetto() const;
    const CicloDomanda* domanda() const { return domanda_ ? &*domanda_ : nullptr; }
    int concorrente_di_turno() const { return turni_.corrente(); }
    int numero_concorrenti() const { return turni_.numero(); }
    const std::vector<CategoriaEstas>& categorie() const { return categorie_; }
    // Oggetti della categoria già portati in gioco (compreso quello corrente).
    std::size_t giocati(int cat) const {
        return categoria_valida(cat) ? prossimo_[static_cast<std::size_t>(cat)] : 0;
    }

    static std::string_view nome(Stato s);

private:
    Esito su_scelta_categoria(const Evento& e, Traccia* t);
    Esito su_attesa_oggetto(const Evento& e, Traccia* t);
    Esito su_oggetto_in_gioco(const Evento& e, Traccia* t);
    Esito su_categoria_finita(const Evento& e, Traccia* t);

    Esito apri_categoria(int indice, TipoEvento ev, Traccia* t);
    Esito apri_oggetto(TipoEvento ev, Traccia* t);
    bool categoria_valida(int i) const { return i >= 0 && i < static_cast<int>(categorie_.size()); }
    bool ha_oggetti(int i) const;
    void vai(Stato nuovo, TipoEvento ev, Traccia* t);

    std::vector<CategoriaEstas> categorie_;
    std::vector<std::size_t> prossimo_;  // per categoria: indice del prossimo oggetto non ancora giocato
    Turni turni_;
    Stato stato_ = Stato::SceltaCategoria;
    int categoria_ = -1;
    std::size_t oggetto_ = 0;
    std::optional<CicloDomanda> domanda_;
};

}  // namespace lascossa
