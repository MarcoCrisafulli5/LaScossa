#include <doctest/doctest.h>

#include "lascossa/ciclo_domanda.hpp"

using namespace lascossa;
using S = CicloDomanda::Stato;

namespace {
Evento sel(int i) { return {TipoEvento::Seleziona, i}; }
const Evento kConferma{TipoEvento::Conferma};
}  // namespace

TEST_CASE("selezione giusta: giallo poi verde") {
    CicloDomanda d(4, 2, false);
    Traccia t;
    CHECK(d.gestisci(sel(2), &t));
    CHECK(d.stato() == S::Selezionata);
    CHECK(d.gestisci(kConferma, &t));
    CHECK(d.stato() == S::Giusta);
    CHECK(d.conclusa());
    REQUIRE(t.size() == 2);
    CHECK(t[1].da == "Selezionata");
    CHECK(t[1].a == "Giusta");
}

TEST_CASE("il conduttore può correggere la selezione prima di confermare") {
    CicloDomanda d(4, 2, false);
    CHECK(d.gestisci(sel(1), nullptr));
    CHECK(d.gestisci(sel(2), nullptr));
    CHECK(d.selezionata() == 2);
    CHECK(d.gestisci(kConferma, nullptr));
    CHECK(d.stato() == S::Giusta);
}

TEST_CASE("conferma senza selezione viene rifiutata con motivo") {
    CicloDomanda d(4, 0, false);
    const Esito r = d.gestisci(kConferma, nullptr);
    CHECK_FALSE(r);
    CHECK(r.motivo == "CONFERMA non ammesso in Mostrata");
    CHECK(d.stato() == S::Mostrata);
}

TEST_CASE("opzione inesistente rifiutata") {
    CicloDomanda d(2, 0, false);
    CHECK_FALSE(d.gestisci(sel(5), nullptr));
    CHECK_FALSE(d.gestisci(sel(-1), nullptr));
}

TEST_CASE("senza riprova: dopo l'errore la domanda è conclusa") {
    CicloDomanda d(2, 0, false);
    CHECK(d.gestisci(sel(1), nullptr));
    CHECK(d.gestisci(kConferma, nullptr));
    CHECK(d.stato() == S::Sbagliata);
    CHECK(d.conclusa());
    CHECK_FALSE(d.gestisci(sel(0), nullptr));
}

TEST_CASE("con riprova (modalità Date): l'opzione sbagliata resta rossa e non si riseleziona") {
    CicloDomanda d(4, 3, true);
    CHECK(d.gestisci(sel(0), nullptr));
    CHECK(d.gestisci(kConferma, nullptr));
    CHECK(d.stato() == S::Sbagliata);
    CHECK_FALSE(d.conclusa());
    CHECK(d.bruciata(0));
    CHECK_FALSE(d.gestisci(sel(0), nullptr));  // già rossa
    CHECK(d.gestisci(sel(3), nullptr));
    CHECK(d.gestisci(kConferma, nullptr));
    CHECK(d.stato() == S::Giusta);
    CHECK(d.bruciata(0));
}

TEST_CASE("dopo Giusta ogni evento è rifiutato") {
    CicloDomanda d(2, 0, false);
    CHECK(d.gestisci(sel(0), nullptr));
    CHECK(d.gestisci(kConferma, nullptr));
    CHECK_FALSE(d.gestisci(sel(1), nullptr));
    CHECK_FALSE(d.gestisci(kConferma, nullptr));
}

TEST_CASE("rivela: da Mostrata o Selezionata porta a Rivelata, stato finale") {
    CicloDomanda d(4, 1, false);
    CHECK(d.gestisci({TipoEvento::Rivela}, nullptr));
    CHECK(d.stato() == S::Rivelata);
    CHECK(d.conclusa());
    CHECK_FALSE(d.gestisci(sel(1), nullptr));

    CicloDomanda d2(4, 1, true);
    CHECK(d2.gestisci(sel(0), nullptr));
    CHECK(d2.gestisci(kConferma, nullptr));
    CHECK(d2.gestisci({TipoEvento::Rivela}, nullptr));  // con riprova si può rivelare dopo un errore
    CHECK(d2.stato() == S::Rivelata);
    CHECK(d2.bruciata(0));
}

TEST_CASE("le modalità a scelta non accettano il giudizio del conduttore") {
    CicloDomanda d(2, 0, false);
    CHECK(d.gestisci(sel(1), nullptr));
    CHECK_FALSE(d.gestisci({TipoEvento::ConfermaGiusta}, nullptr));
}
