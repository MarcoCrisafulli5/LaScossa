#include <doctest/doctest.h>

#include <nlohmann/json.hpp>

#include "cartella_temporanea.hpp"
#include "regia.hpp"

using namespace lascossa;
using namespace lascossa::server;
using nlohmann::json;

namespace {

constexpr IdClient kPlancia = 1;
constexpr IdClient kPubblico = 2;

void prepara(const test::CartellaTemporanea& d, std::string_view config = R"({"concorrenti":{"numero":3}})") {
    d.scrivi("config.json", config);
    d.scrivi("contenuti/mod1_estas.csv",
             "categoria;oggetto;appartiene\nPiatti tipici regionali;Arancini;si\nPiatti tipici regionali;Sushi;no\n");
}

// Messaggi ricevuti da un client in una risposta.
std::vector<json> per(const std::vector<Uscita>& out, IdClient c) {
    std::vector<json> r;
    for (const auto& u : out) {
        if (u.destinatario == c) r.push_back(json::parse(u.testo));
    }
    return r;
}

std::string evento(const std::string& id, const std::string& nome, int arg = -1) {
    return json{{"tipo", "event"}, {"id", id}, {"evento", nome}, {"arg", arg}}.dump();
}

struct Sala {
    Regia regia;
    int contatore = 0;
    explicit Sala(const test::CartellaTemporanea& d, bool nuova = false) : regia(d.percorso(), nuova) {
        regia.connesso(kPlancia);
        regia.connesso(kPubblico);
        regia.messaggio(kPlancia, R"({"tipo":"hello","ruolo":"plancia"})", 0);
        regia.messaggio(kPubblico, R"({"tipo":"hello","ruolo":"pubblico"})", 0);
    }
    std::vector<Uscita> invia(const std::string& nome, int arg = -1) {
        return regia.messaggio(kPlancia, evento("e" + std::to_string(++contatore), nome, arg), 1000);
    }
};

}  // namespace

TEST_CASE("hello: snapshot per ruolo; il pubblico non può inviare eventi") {
    test::CartellaTemporanea d;
    prepara(d);
    Regia r(d.percorso(), false);
    r.connesso(kPubblico);
    auto m = per(r.messaggio(kPubblico, R"({"tipo":"hello","ruolo":"pubblico"})", 5), kPubblico);
    REQUIRE(m.size() == 1);
    CHECK(m[0]["tipo"] == "snapshot");
    CHECK(m[0]["vista"]["modalita"].is_null());
    CHECK_FALSE(m[0]["vista"].contains("ammessi"));

    m = per(r.messaggio(kPubblico, evento("x", "AVVIA_SERATA"), 5), kPubblico);
    REQUIRE(m.size() == 1);
    CHECK(m[0]["tipo"] == "reject");
    CHECK(r.partita().registro().empty());
}

TEST_CASE("partita: ack, aggiornamenti a entrambi, soluzione mai al pubblico") {
    test::CartellaTemporanea d;
    prepara(d);
    Sala s(d);

    auto out = s.invia("AVVIA_SERATA");
    auto plancia = per(out, kPlancia);
    REQUIRE(plancia.size() == 2);
    CHECK(plancia[0]["tipo"] == "ack");
    CHECK(plancia[1]["tipo"] == "update");
    CHECK(plancia[1]["vista"]["statoSerata"] == "SceltaModalita");
    CHECK(per(out, kPubblico).size() == 1);

    s.invia("SCEGLI_MODALITA", 0);
    out = s.invia("SCEGLI_CATEGORIA", 0);
    CHECK(per(out, kPlancia).back()["vista"]["estas"]["anteprima"] == "Arancini");
    const std::string attesa = per(out, kPubblico).back().dump();
    CHECK(attesa.find("Piatti tipici regionali") != std::string::npos);
    CHECK(attesa.find("Arancini") == std::string::npos);  // l'oggetto in arrivo resta in plancia

    out = s.invia("MOSTRA_DOMANDA");
    const json vp = per(out, kPlancia).back()["vista"];
    CHECK(vp["estas"]["oggetto"] == "Arancini");
    CHECK(vp["estas"]["oggettoAppartiene"] == true);

    const std::string pubblico = per(out, kPubblico).back().dump();
    CHECK(pubblico.find("Arancini") != std::string::npos);
    CHECK(pubblico.find("Sushi") == std::string::npos);  // oggetti futuri
    CHECK(pubblico.find("appartiene") == std::string::npos);
    CHECK(pubblico.find("corretta") == std::string::npos);
    CHECK(pubblico.find("soluzione") == std::string::npos);
}

TEST_CASE("evento rifiutato: reject con motivo, registro invariato, plancia aggiornata") {
    test::CartellaTemporanea d;
    prepara(d);
    Sala s(d);
    const auto out = s.invia("CONFERMA");
    const auto plancia = per(out, kPlancia);
    REQUIRE(plancia.size() == 2);
    CHECK(plancia[0]["tipo"] == "reject");
    CHECK(plancia[0]["motivo"] == "CONFERMA non ammesso in Lobby");
    CHECK(plancia[1]["vista"]["debug"]["ultimiRifiutati"].size() == 1);
    CHECK(per(out, kPubblico).empty());
    CHECK(s.regia.partita().registro().empty());
}

TEST_CASE("doppio invio dello stesso id: confermato ma applicato una volta") {
    test::CartellaTemporanea d;
    prepara(d);
    Sala s(d);
    s.regia.messaggio(kPlancia, evento("uguale", "PAUSA_GENERALE"), 1);
    const auto m = per(s.regia.messaggio(kPlancia, evento("uguale", "PAUSA_GENERALE"), 2), kPlancia);
    REQUIRE(m.size() == 1);
    CHECK(m[0]["tipo"] == "ack");
    CHECK(s.regia.partita().registro().size() == 1);
}

TEST_CASE("ripresa dopo un crash: stesso stato, registro che continua") {
    test::CartellaTemporanea d;
    prepara(d);
    std::filesystem::path file;
    {
        Sala s(d);
        s.invia("AVVIA_SERATA");
        s.invia("SCEGLI_MODALITA", 0);
        s.invia("SCEGLI_CATEGORIA", 0);
        s.invia("MOSTRA_DOMANDA");
        s.invia("SELEZIONA", 1);
        s.invia("ANNULLA");
        file = s.regia.file_registro();
    }
    Sala ripresa(d);
    CHECK(ripresa.regia.file_registro() == file);
    CHECK(ripresa.regia.partita().serata().stato() == Serata::Stato::InGioco);
    CHECK(ripresa.regia.partita().registro().size() == 4);  // il SELEZIONA è stato annullato
    ripresa.invia("SELEZIONA", 0);
    CHECK(RegistroSuFile::leggi(file).eventi.size() == 7);
}

TEST_CASE("nuova partita: rifiutata in gioco, ammessa nella lobby; serata chiusa non si riprende") {
    test::CartellaTemporanea d;
    prepara(d);
    Sala s(d);
    s.invia("AVVIA_SERATA");
    s.invia("SCEGLI_MODALITA", 0);
    CHECK(per(s.invia("NUOVA_PARTITA"), kPlancia)[0]["tipo"] == "reject");

    s.invia("FINE_MODALITA");
    s.invia("CHIUDI_SERATA");
    const auto prima = s.regia.file_registro();
    CHECK(per(s.invia("NUOVA_PARTITA"), kPlancia)[0]["tipo"] == "ack");
    CHECK(s.regia.file_registro() != prima);
    CHECK(s.regia.partita().serata().stato() == Serata::Stato::Lobby);

    s.invia("AVVIA_SERATA");
    s.invia("CHIUDI_SERATA");
    Sala dopo(d);  // l'ultima partita è chiusa: se ne apre una nuova
    CHECK(dopo.regia.partita().serata().stato() == Serata::Stato::Lobby);
    CHECK(dopo.regia.partita().registro().empty());
}

TEST_CASE("PIN della plancia") {
    test::CartellaTemporanea d;
    prepara(d, R"({"plancia":{"pin":"1234"}})");
    Regia r(d.percorso(), false);
    r.connesso(kPlancia);
    CHECK(per(r.messaggio(kPlancia, R"({"tipo":"hello","ruolo":"plancia","pin":"0000"})", 0), kPlancia)[0]["tipo"] ==
          "reject");
    CHECK(per(r.messaggio(kPlancia, evento("a", "AVVIA_SERATA"), 0), kPlancia)[0]["tipo"] == "reject");
    CHECK(per(r.messaggio(kPlancia, R"({"tipo":"hello","ruolo":"plancia","pin":"1234"})", 0), kPlancia)[0]["tipo"] ==
          "snapshot");
}

TEST_CASE("messaggi malformati e ping") {
    test::CartellaTemporanea d;
    prepara(d);
    Regia r(d.percorso(), false);
    r.connesso(kPlancia);
    CHECK(per(r.messaggio(kPlancia, "{rotto", 0), kPlancia)[0]["tipo"] == "reject");
    CHECK(per(r.messaggio(kPlancia, R"({"tipo":"boh"})", 0), kPlancia)[0]["tipo"] == "reject");
    const auto pong = per(r.messaggio(kPlancia, R"({"tipo":"ping"})", 77), kPlancia);
    CHECK(pong[0]["tipo"] == "pong");
    CHECK(pong[0]["oraServer"] == 77);
}

namespace {
// ESA con tempi brevi: 3 indizi da 1 s, poi 2 s di risposta -> scade 5 s dopo MOSTRA_DOMANDA.
void prepara_esa(const test::CartellaTemporanea& d) {
    prepara(d, R"({"concorrenti":{"numero":3},"esa":{"tempoLetturaIndizioSec":1,"timerRispostaSec":2}})");
    d.scrivi("contenuti/mod2_esa.csv",
             "indizio1;indizio2;indizio3;soluzione\nRoma;Cesare;Rubicone;Il dado è tratto\n");
}
}  // namespace

TEST_CASE("ESA: TIMER_SCADUTO lo genera il server; la frase arriva al pubblico solo dopo") {
    test::CartellaTemporanea d;
    prepara_esa(d);
    Sala s(d, true);
    s.invia("AVVIA_SERATA");
    s.invia("SCEGLI_MODALITA", static_cast<int>(IdModalita::Esa));
    auto out = s.regia.messaggio(kPlancia, evento("m", "MOSTRA_DOMANDA"), 1000);
    REQUIRE(per(out, kPubblico).size() == 1);
    const json pubblico = per(out, kPubblico)[0]["vista"]["esa"];
    CHECK(pubblico["indizi"].size() == 3);
    CHECK(pubblico["timer"]["stato"] == "corre");
    CHECK(pubblico["timer"]["scadenzaMs"] == 6000);
    CHECK(per(out, kPubblico)[0].dump().find("dado") == std::string::npos);  // la soluzione non c'è

    // la plancia non può far scadere il timer
    auto r = per(s.invia("TIMER_SCADUTO"), kPlancia);
    REQUIRE_FALSE(r.empty());
    CHECK(r[0]["tipo"] == "reject");

    REQUIRE(s.regia.prossima_scadenza().has_value());
    CHECK(*s.regia.prossima_scadenza() == 6000);
    CHECK(s.regia.scadenza(5999).empty());  // non ancora

    out = s.regia.scadenza(6000);
    REQUIRE(per(out, kPubblico).size() == 1);
    CHECK(per(out, kPubblico)[0]["vista"]["esa"]["stato"] == "Scaduta");
    CHECK(per(out, kPubblico)[0]["vista"]["esa"]["frase"] == "Il dado è tratto");
    CHECK(s.regia.partita().registro().back().tipo == TipoEvento::TimerScaduto);
    CHECK_FALSE(s.regia.prossima_scadenza().has_value());
    CHECK(s.regia.scadenza(9000).empty());  // una volta sola
}

TEST_CASE("ESA: ripresa dopo un crash con un timer che correva -> pausa generale") {
    test::CartellaTemporanea d;
    prepara_esa(d);
    {
        Sala s(d, true);
        s.invia("AVVIA_SERATA");
        s.invia("SCEGLI_MODALITA", static_cast<int>(IdModalita::Esa));
        s.regia.messaggio(kPlancia, evento("m", "MOSTRA_DOMANDA"), 1000);
    }  // crash

    Regia r(d.percorso(), false);
    CHECK(r.partita().serata().in_pausa());
    CHECK_FALSE(r.prossima_scadenza().has_value());
    CHECK(r.partita().registro().back().arg == kPausaAutomatica);
    bool avvisato = false;
    for (const auto& a : r.avvisi()) avvisato = avvisato || a.find("pausa generale") != std::string::npos;
    CHECK(avvisato);

    // la pausa automatica è nel registro: un'altra ripresa ritrova lo stesso stato, senza doppioni
    Regia r2(d.percorso(), false);
    CHECK(r2.partita().serata().in_pausa());
    CHECK(r2.partita().registro().size() == r.partita().registro().size());
}
