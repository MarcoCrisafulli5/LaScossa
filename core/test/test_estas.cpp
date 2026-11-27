#include <doctest/doctest.h>

#include "lascossa/estas.hpp"

using namespace lascossa;
using S = Estas::Stato;

namespace {
std::vector<CategoriaEstas> categorie() {
    return {
        {"Piatti tipici regionali", {{"Arancini", true}, {"Sushi", false}}},
        {"Fiumi italiani", {{"Po", true}}},
    };
}
Evento ev(TipoEvento t, int arg = -1) { return {t, arg}; }
}  // namespace

TEST_CASE("partita base: categoria, attesa, oggetto, risposta giusta, attesa, oggetto successivo") {
    Estas m(categorie(), 3);
    CHECK(m.stato() == S::SceltaCategoria);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.stato() == S::AttesaOggetto);
    CHECK(m.oggetto_corrente() == nullptr);  // il pubblico vede solo la categoria
    REQUIRE(m.prossimo_oggetto() != nullptr);
    CHECK(m.prossimo_oggetto()->nome == "Arancini");
    CHECK(m.giocati(0) == 0);

    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.stato() == S::OggettoInGioco);
    CHECK(m.oggetto_corrente()->nome == "Arancini");
    CHECK(m.prossimo_oggetto() == nullptr);
    CHECK(m.giocati(0) == 1);

    CHECK(m.gestisci(ev(TipoEvento::Seleziona, Estas::kEstas), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Conferma), nullptr));
    CHECK(m.domanda()->stato() == CicloDomanda::Stato::Giusta);
    CHECK(m.concorrente_di_turno() == 0);  // risposta giusta: il turno resta

    CHECK(m.gestisci(ev(TipoEvento::ProssimaDomanda), nullptr));
    CHECK(m.stato() == S::AttesaOggetto);
    CHECK(m.domanda() == nullptr);
    CHECK(m.prossimo_oggetto()->nome == "Sushi");
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.oggetto_corrente()->nome == "Sushi");
}

TEST_CASE("attesa: niente risposte prima di mostrare l'oggetto") {
    Estas m(categorie(), 3);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::Seleziona, Estas::kEstas), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::Conferma), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::Rivela), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ProssimaDomanda), nullptr));
    CHECK(m.stato() == S::AttesaOggetto);
}

TEST_CASE("attesa: cambiare categoria non consuma l'oggetto in arrivo") {
    Estas m(categorie(), 3);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));  // già scelta
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 1), nullptr));
    CHECK(m.prossimo_oggetto()->nome == "Po");
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.prossimo_oggetto()->nome == "Arancini");
    CHECK(m.giocati(0) == 0);
    CHECK(m.giocati(1) == 0);
}

TEST_CASE("risposta sbagliata: rosso e turno al concorrente successivo") {
    Estas m(categorie(), 3);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ProssimaDomanda), nullptr));  // manca l'esito
    CHECK_FALSE(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));     // oggetto già a schermo
    CHECK(m.gestisci(ev(TipoEvento::Seleziona, Estas::kNonEstas), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Conferma), nullptr));
    CHECK(m.domanda()->stato() == CicloDomanda::Stato::Sbagliata);
    CHECK(m.concorrente_di_turno() == 1);
}

TEST_CASE("cambio categoria a domanda conclusa, non prima, passando dall'attesa") {
    Estas m(categorie(), 2);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ScegliCategoria, 1), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Seleziona, Estas::kEstas), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Conferma), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 1), nullptr));
    CHECK(m.stato() == S::AttesaOggetto);
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.oggetto_corrente()->nome == "Po");
}

TEST_CASE("fine oggetti: CategoriaFinita, poi si riprende un'altra categoria dal punto lasciato") {
    Estas m(categorie(), 2);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 1), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Seleziona, Estas::kEstas), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Conferma), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::ProssimaDomanda), nullptr));
    CHECK(m.stato() == S::CategoriaFinita);
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ScegliCategoria, 1), nullptr));  // esaurita
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.oggetto_corrente()->nome == "Arancini");
}

TEST_CASE("passa turno manuale, anche durante l'attesa") {
    Estas m(categorie(), 2);
    CHECK(m.gestisci(ev(TipoEvento::ScegliCategoria, 0), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::PassaTurno), nullptr));
    CHECK(m.concorrente_di_turno() == 1);
    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::PassaTurno), nullptr));
    CHECK(m.concorrente_di_turno() == 0);
}
