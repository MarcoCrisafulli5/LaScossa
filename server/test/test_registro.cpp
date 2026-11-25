#include <doctest/doctest.h>

#include <chrono>

#include "cartella_temporanea.hpp"
#include "csv.hpp"
#include "registro_file.hpp"

using namespace lascossa;
using namespace lascossa::server;

TEST_CASE("registro su file: scrittura, rilettura, ripresa in fondo") {
    test::CartellaTemporanea d;
    const auto cartella = d.percorso() / "partite";
    CHECK_FALSE(RegistroSuFile::ultimo(cartella).has_value());

    std::filesystem::path file;
    {
        auto r = RegistroSuFile::nuovo(cartella, std::chrono::system_clock::now());
        file = r.file();
        Evento e{TipoEvento::AggiornaRiquadro};
        e.testo = "Anna 1000; \"Bruno\" 500";
        e.t_ms = 42;
        r.aggiungi(Evento{TipoEvento::AvviaSerata}, "Lobby");
        r.aggiungi(e, "SceltaModalita");
        CHECK(r.righe() == 2);
    }
    CHECK(RegistroSuFile::ultimo(cartella) == file);

    auto letti = RegistroSuFile::leggi(file);
    REQUIRE(letti.eventi.size() == 2);
    CHECK(letti.eventi[0].tipo == TipoEvento::AvviaSerata);
    CHECK(letti.eventi[1].testo == "Anna 1000; \"Bruno\" 500");
    CHECK(letti.eventi[1].t_ms == 42);
    CHECK_FALSE(letti.avviso.has_value());

    {
        auto r = RegistroSuFile::riprendi(file, letti.eventi.size());
        r.aggiungi(Evento{TipoEvento::ChiudiSerata}, "SceltaModalita");
        CHECK(r.righe() == 3);
    }
    CHECK(RegistroSuFile::leggi(file).eventi.size() == 3);
}

TEST_CASE("registro su file: ultima riga troncata ignorata, riga rotta in mezzo è un errore") {
    test::CartellaTemporanea d;
    d.scrivi("p.jsonl", "{\"evento\":\"AVVIA_SERATA\"}\n{\"evento\":\"SCEGLI_MOD");
    auto letti = RegistroSuFile::leggi(d.percorso() / "p.jsonl");
    CHECK(letti.eventi.size() == 1);
    CHECK(letti.avviso.has_value());

    d.scrivi("q.jsonl", "{\"evento\":\"AVV\n{\"evento\":\"AVVIA_SERATA\"}\n");
    CHECK_THROWS_AS(RegistroSuFile::leggi(d.percorso() / "q.jsonl"), ErroreDati);
}
