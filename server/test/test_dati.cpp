#include <doctest/doctest.h>

#include <algorithm>

#include "cartella_temporanea.hpp"
#include "csv.hpp"
#include "dati.hpp"

using namespace lascossa;
using namespace lascossa::server;

TEST_CASE("csv: punto e virgola, BOM, CRLF, righe vuote, spazi ai lati") {
    const auto t = leggi_csv("\xEF\xBB\xBF" "Categoria ; Oggetto;appartiene\r\nPiatti;Arancini;si\r\n\r\n;;\r\nPiatti; Sushi ;no", "x.csv");
    REQUIRE(t.intestazione.size() == 3);
    CHECK(t.intestazione[0] == "categoria");
    CHECK(t.intestazione[1] == "oggetto");
    REQUIRE(t.righe.size() == 2);
    CHECK(t.righe[0].numero == 2);
    CHECK(t.righe[1].numero == 5);
    CHECK(t.righe[1].campi[1] == "Sushi");
}

TEST_CASE("csv: campi tra virgolette con punto e virgola e virgolette doppie") {
    const auto t = leggi_csv("a;b\n\"x; y\";\"di \"\"lui\"\"\"\n", "x.csv");
    REQUIRE(t.righe.size() == 1);
    CHECK(t.righe[0].campi[0] == "x; y");
    CHECK(t.righe[0].campi[1] == "di \"lui\"");
}

TEST_CASE("csv: file non UTF-8 (Excel in Windows-1252) dà un errore chiaro") {
    CHECK_THROWS_WITH_AS(leggi_csv("a;b\nPerch\xE9;no\n", "mod1_estas.csv"),
                         doctest::Contains("non è in UTF-8"), ErroreDati);
}

TEST_CASE("estas: categorie nell'ordine del file, si/sì/no") {
    std::vector<std::string> avvisi;
    const auto t = leggi_csv("categoria;oggetto;appartiene\nPiatti;Arancini;si\nFiumi;Po;Sì\nPiatti;Sushi;NO\n", "e.csv");
    const auto c = leggi_estas(t, "e.csv", avvisi);
    REQUIRE(c.size() == 2);
    CHECK(c[0].nome == "Piatti");
    REQUIRE(c[0].oggetti.size() == 2);
    CHECK(c[0].oggetti[0].appartiene);
    CHECK_FALSE(c[0].oggetti[1].appartiene);
    CHECK(c[1].oggetti[0].appartiene);
    CHECK(avvisi.size() == 2);  // entrambe con meno di 11 oggetti
}

TEST_CASE("estas: errori con file e riga") {
    std::vector<std::string> avvisi;
    CHECK_THROWS_WITH_AS(leggi_estas(leggi_csv("categoria;oggetto;appartiene\nPiatti;Arancini;forse\n", "e.csv"), "e.csv", avvisi),
                         "e.csv riga 2: appartiene deve essere si o no, trovato \"forse\"", ErroreDati);
    CHECK_THROWS_WITH_AS(leggi_estas(leggi_csv("categoria;oggetto\nPiatti;Arancini\n", "e.csv"), "e.csv", avvisi),
                         doctest::Contains("manca la colonna \"appartiene\""), ErroreDati);
    CHECK_THROWS_AS(leggi_estas(leggi_csv("categoria;oggetto;appartiene\nPiatti;;si\n", "e.csv"), "e.csv", avvisi), ErroreDati);
}

TEST_CASE("config: valori, PIN e errori") {
    Configurazione cfg;
    Impostazioni imp;
    leggi_config(R"({"concorrenti":{"numero":6},"plancia":{"pin":"1234"},"esa":{"timerRispostaSec":30}})", cfg, imp);
    CHECK(cfg.numero_concorrenti == 6);
    CHECK(cfg.estas.attesa_esito_ms == 2000);  // default se manca in config.json
    REQUIRE(imp.pin.has_value());
    CHECK(*imp.pin == "1234");

    leggi_config(R"({"plancia":{"pin":null}})", cfg, imp);
    CHECK_FALSE(imp.pin.has_value());

    leggi_config(R"({"estas":{"attesaEsitoMs":0}})", cfg, imp);
    CHECK(cfg.estas.attesa_esito_ms == 0);  // 0 = esito subito, senza suspense

    CHECK_THROWS_AS(leggi_config(R"({"concorrenti":{"numero":0}})", cfg, imp), ErroreDati);
    CHECK_THROWS_WITH_AS(leggi_config(R"({"estas":{"attesaEsitoMs":-1}})", cfg, imp),
                         doctest::Contains("estas.attesaEsitoMs"), ErroreDati);
    CHECK_THROWS_AS(leggi_config(R"({"estas":{"attesaEsitoMs":"2s"}})", cfg, imp), ErroreDati);
    CHECK_THROWS_AS(leggi_config("{non json", cfg, imp), ErroreDati);
}

TEST_CASE("esa: frasi valide, avvisi ed errori con file e riga") {
    std::vector<std::string> avvisi;
    const auto frasi = leggi_esa(leggi_csv("soluzione;indizio1;indizio2;indizio3\n"
                                           "Il dado è tratto;Roma;Cesare;Rubicone\n"
                                           "Il dado è tratto;Alea;Iacta;Est\n",
                                           "e.csv"),
                                 "e.csv", avvisi);
    REQUIRE(frasi.size() == 2);
    CHECK(frasi[0].indizi[2] == "Rubicone");  // ordine delle colonne libero
    CHECK(frasi[0].soluzione == "Il dado è tratto");
    REQUIRE(avvisi.size() == 1);
    CHECK(avvisi[0].find("ripetuta") != std::string::npos);

    CHECK_THROWS_WITH_AS(
        leggi_esa(leggi_csv("indizio1;indizio2;indizio3;soluzione\nRoma;;Rubicone;X\n", "e.csv"), "e.csv", avvisi),
        "e.csv riga 2: indizio2 vuoto", ErroreDati);
    CHECK_THROWS_WITH_AS(
        leggi_esa(leggi_csv("indizio1;indizio2;indizio3;soluzione\nA;B;C;\n", "e.csv"), "e.csv", avvisi),
        "e.csv riga 2: soluzione vuota", ErroreDati);
    CHECK_THROWS_WITH_AS(leggi_esa(leggi_csv("indizio1;indizio2;soluzione\nA;B;C\n", "e.csv"), "e.csv", avvisi),
                         doctest::Contains("indizio3"), ErroreDati);
}

TEST_CASE("config: tempi di ESA in secondi") {
    Configurazione cfg;
    Impostazioni imp;
    CHECK(cfg.esa.lettura_indizio_ms == 3000);
    leggi_config(R"({"esa":{"tempoLetturaIndizioSec":2,"timerRispostaSec":45}})", cfg, imp);
    CHECK(cfg.esa.lettura_indizio_ms == 2000);
    CHECK(cfg.esa.timer_risposta_ms == 45000);
    CHECK_THROWS_WITH_AS(leggi_config(R"({"esa":{"timerRispostaSec":0}})", cfg, imp),
                         doctest::Contains("esa.timerRispostaSec"), ErroreDati);
}

TEST_CASE("carica_dati: cartella completa e file mancanti") {
    test::CartellaTemporanea d;
    CHECK_THROWS_WITH_AS(carica_dati(d.percorso()), doctest::Contains("config.json"), ErroreDati);
    d.scrivi("config.json", R"({"concorrenti":{"numero":3}})");
    CHECK_THROWS_WITH_AS(carica_dati(d.percorso()), doctest::Contains("mod1_estas.csv"), ErroreDati);
    d.scrivi("contenuti/mod1_estas.csv", "categoria;oggetto;appartiene\nPiatti;Arancini;si\n");
    const DatiSerata dati = carica_dati(d.percorso());
    CHECK(dati.configurazione.numero_concorrenti == 3);
    CHECK(dati.contenuti.estas.size() == 1);
    CHECK(dati.contenuti.esa.empty());  // mod2_esa.csv facoltativo: solo un avviso
    CHECK(std::any_of(dati.avvisi.begin(), dati.avvisi.end(),
                      [](const std::string& a) { return a.find("mod2_esa.csv") != std::string::npos; }));
}
