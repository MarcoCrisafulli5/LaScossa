#include <doctest/doctest.h>

#include <chrono>
#include <string>

#include "lascossa/partita.hpp"

using namespace lascossa;
using S = Serata::Stato;

namespace {
Contenuti contenuti() {
    Contenuti c;
    c.estas = {{"Piatti tipici regionali", {{"Arancini", true}, {"Sushi", false}}}};
    return c;
}
Evento ev(TipoEvento t, int arg = -1) { return {t, arg}; }
const int kEstas = static_cast<int>(IdModalita::Estas);
}  // namespace

TEST_CASE("serata: lobby, scelta modalità, gioco, fine modalità, chiusura") {
    Partita p({3}, contenuti());
    CHECK(p.serata().stato() == S::Lobby);
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK(p.serata().stato() == S::InGioco);
    CHECK(p.serata().modalita()->id() == IdModalita::Estas);
    CHECK(p.applica(ev(TipoEvento::FineModalita)));
    CHECK(p.serata().stato() == S::SceltaModalita);
    CHECK(p.applica(ev(TipoEvento::ChiudiSerata)));
    CHECK(p.serata().stato() == S::Chiusa);
    CHECK(p.registro().size() == 4);
}

TEST_CASE("modalità non ancora implementata: rifiuto con motivo") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    const Esito r = p.applica(ev(TipoEvento::ScegliModalita, static_cast<int>(IdModalita::Duelli)));
    CHECK_FALSE(r);
    CHECK(p.rifiutati().size() == 1);
    CHECK(p.registro().size() == 1);  // i rifiutati non entrano nel registro
}

TEST_CASE("pausa generale: blocca gli eventi di gioco, non la fine modalità") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK(p.applica(ev(TipoEvento::PausaGenerale)));
    CHECK_FALSE(p.applica(ev(TipoEvento::PausaGenerale)));  // già in pausa
    CHECK_FALSE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    CHECK(p.applica(ev(TipoEvento::RiprendiGenerale)));
    CHECK(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
}

TEST_CASE("annulla: torna allo stato precedente rigiocando il registro") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    CHECK(p.applica(ev(TipoEvento::Seleziona, Estas::kNonEstas)));  // clic sbagliato del conduttore
    CHECK(p.applica(ev(TipoEvento::Conferma)));

    const auto* estas = dynamic_cast<const Estas*>(p.serata().modalita());
    REQUIRE(estas != nullptr);
    CHECK(estas->domanda()->stato() == CicloDomanda::Stato::Sbagliata);

    CHECK(p.applica(ev(TipoEvento::Annulla)));  // annulla la conferma
    CHECK(p.applica(ev(TipoEvento::Annulla)));  // annulla la selezione
    estas = dynamic_cast<const Estas*>(p.serata().modalita());
    REQUIRE(estas != nullptr);
    CHECK(estas->domanda()->stato() == CicloDomanda::Stato::Mostrata);
    CHECK(estas->concorrente_di_turno() == 0);
    CHECK(p.registro().size() == 3);
}

TEST_CASE("annulla con registro vuoto è rifiutato") {
    Partita p({3}, contenuti());
    CHECK_FALSE(p.applica(ev(TipoEvento::Annulla)));
}

TEST_CASE("la traccia registra ogni transizione con macchina, stati ed evento") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    CHECK(p.applica(ev(TipoEvento::Seleziona, Estas::kEstas)));
    REQUIRE(p.traccia().size() == 4);
    CHECK(p.traccia()[0].macchina == "Serata");
    CHECK(p.traccia()[2].macchina == "Estas");
    CHECK(p.traccia()[3].macchina == "Domanda");
    CHECK(p.traccia()[3].a == "Selezionata");
}

TEST_CASE("replay di 10.000 eventi: misura per decidere sulle istantanee") {
    Contenuti c;
    CategoriaEstas cat{"Grande", {}};
    for (int i = 0; i < 5000; ++i) cat.oggetti.push_back({"o" + std::to_string(i), i % 2 == 0});
    c.estas = {cat};

    std::vector<Evento> eventi = {ev(TipoEvento::AvviaSerata), ev(TipoEvento::ScegliModalita, kEstas),
                                  ev(TipoEvento::ScegliCategoria, 0)};
    while (eventi.size() < 10'000) {
        eventi.push_back(ev(TipoEvento::Seleziona, Estas::kEstas));
        eventi.push_back(ev(TipoEvento::Conferma));
        eventi.push_back(ev(TipoEvento::ProssimaDomanda));
    }
    Partita p({4}, c);
    const auto inizio = std::chrono::steady_clock::now();
    p.rigioca(eventi);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - inizio).count();
    MESSAGE("replay di " << eventi.size() << " eventi: " << ms << " ms");
    CHECK(ms < 100);
}

TEST_CASE("riquadro punteggio: aggiorna, mostra, nascondi in ogni stato aperto") {
    Partita p({3}, contenuti());
    Evento agg{TipoEvento::AggiornaRiquadro};
    agg.testo = "Anna 1000 - Bruno 500";
    CHECK(p.applica(agg));
    CHECK(p.serata().riquadro_testo() == "Anna 1000 - Bruno 500");
    CHECK(p.applica(ev(TipoEvento::MostraRiquadro)));
    CHECK(p.serata().riquadro_visibile());
    CHECK_FALSE(p.applica(ev(TipoEvento::MostraRiquadro)));
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::NascondiRiquadro)));
    CHECK_FALSE(p.serata().riquadro_visibile());
}

TEST_CASE("ricarica contenuti solo fuori dal gioco") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::RicaricaContenuti)));
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::RicaricaContenuti)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK_FALSE(p.applica(ev(TipoEvento::RicaricaContenuti)));
}

TEST_CASE("rivela in Estas passa al ciclo della domanda") {
    Partita p({3}, contenuti());
    CHECK(p.applica(ev(TipoEvento::AvviaSerata)));
    CHECK(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    CHECK(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    CHECK(p.applica(ev(TipoEvento::Rivela)));
    const auto* estas = dynamic_cast<const Estas*>(p.serata().modalita());
    REQUIRE(estas != nullptr);
    CHECK(estas->domanda()->stato() == CicloDomanda::Stato::Rivelata);
    CHECK(p.applica(ev(TipoEvento::ProssimaDomanda)));
}
