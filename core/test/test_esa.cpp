#include <doctest/doctest.h>

#include "lascossa/esa.hpp"
#include "lascossa/partita.hpp"
#include "lascossa/viste.hpp"

using namespace lascossa;
using S = Esa::Stato;

namespace {
constexpr std::int64_t kLettura = 3000;
constexpr std::int64_t kRisposta = 30000;
constexpr std::int64_t kTotale = 3 * kLettura + kRisposta;  // indizi + conto alla rovescia

std::vector<FraseEsa> frasi() {
    return {
        {{"Roma", "Cesare", "Rubicone"}, "Il dado è tratto"},
        {{"Galileo", "Pisa", "Terra"}, "Eppur si muove"},
    };
}
Evento ev(TipoEvento t, std::int64_t ora = 0, int arg = -1) { return {t, arg, ora}; }

Partita in_gioco_esa() {
    Configurazione cfg;
    cfg.esa = {static_cast<int>(kLettura), static_cast<int>(kRisposta)};
    Contenuti c;
    c.estas = {{"Fiumi italiani", {{"Po", true}}}};
    c.esa = frasi();
    Partita p(cfg, c);
    REQUIRE(p.applica(ev(TipoEvento::AvviaSerata)));
    REQUIRE(p.applica(ev(TipoEvento::ScegliModalita, 0, static_cast<int>(IdModalita::Esa))));
    return p;
}
const Esa& esa(const Partita& p) { return dynamic_cast<const Esa&>(*p.serata().modalita()); }
}  // namespace

TEST_CASE("timer di gioco: ritardo, pausa, ripresa, fermo e scadenza") {
    TimerGioco t;
    CHECK(t.stato() == TimerGioco::Stato::Spento);
    CHECK_FALSE(t.prossima_scadenza().has_value());

    t.avvia(1000, 30000, 9000);
    CHECK(t.totale_ms() == 39000);
    REQUIRE(t.prossima_scadenza().has_value());
    CHECK(*t.prossima_scadenza() == 40000);

    t.pausa(5000);  // durante gli indizi: si conserva anche il ritardo che restava
    CHECK(t.stato() == TimerGioco::Stato::InPausa);
    CHECK(t.residuo_ms() == 35000);
    CHECK_FALSE(t.prossima_scadenza().has_value());
    t.riprendi(100000);
    CHECK(*t.prossima_scadenza() == 135000);

    CHECK_FALSE(t.scadi(134999));  // troppo presto
    CHECK(t.scadi(135000));
    CHECK(t.stato() == TimerGioco::Stato::Scaduto);
    CHECK(t.residuo_ms() == 0);

    TimerGioco f;
    f.avvia(0, 1000);
    f.ferma(400);
    CHECK(f.stato() == TimerGioco::Stato::Fermo);
    CHECK(f.residuo_ms() == 600);
    CHECK_FALSE(f.scadi(5000));  // un timer fermo non scade più
}

TEST_CASE("ESA: attesa, indizi e timer, scadenza, giusta allo scadere, rivela, fine") {
    Esa m(frasi(), kLettura, kRisposta);
    CHECK(m.stato() == S::AttesaFrase);
    CHECK(m.frase_corrente() == nullptr);
    REQUIRE(m.prossima_frase() != nullptr);
    CHECK(m.prossima_frase()->soluzione == "Il dado è tratto");

    CHECK_FALSE(m.gestisci(ev(TipoEvento::Rivela), nullptr));  // niente frase in gioco

    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda, 1000), nullptr));
    CHECK(m.stato() == S::InGioco);
    CHECK(m.giocate() == 1);
    CHECK_FALSE(m.frase_rivelata());
    // il conto alla rovescia parte dopo i 3 indizi
    CHECK(*m.prossima_scadenza() == 1000 + kTotale);

    // una risposta sbagliata non chiude la frase
    const Esito sbagliata = m.gestisci(ev(TipoEvento::ConfermaSbagliata, 5000), nullptr);
    CHECK_FALSE(sbagliata);
    CHECK(sbagliata.motivo.find("non chiude la frase") != std::string::npos);

    CHECK_FALSE(m.gestisci(ev(TipoEvento::TimerScaduto, 1000 + kTotale - 1), nullptr));
    Traccia t;
    CHECK(m.gestisci(ev(TipoEvento::TimerScaduto, 1000 + kTotale), &t));
    CHECK(m.stato() == S::Scaduta);
    CHECK(m.frase_rivelata());
    REQUIRE(t.size() == 1);
    CHECK(t[0].da == "InGioco");
    CHECK(t[0].a == "Scaduta");

    // la risposta era arrivata allo scadere: il conduttore la dà per giusta
    CHECK(m.gestisci(ev(TipoEvento::ConfermaGiusta, 50000), nullptr));
    CHECK(m.stato() == S::Indovinata);
    CHECK_FALSE(m.gestisci(ev(TipoEvento::ConfermaGiusta, 50001), nullptr));

    CHECK(m.gestisci(ev(TipoEvento::ProssimaDomanda, 60000), nullptr));
    CHECK(m.stato() == S::AttesaFrase);
    CHECK(m.timer().stato() == TimerGioco::Stato::Spento);

    CHECK(m.gestisci(ev(TipoEvento::MostraDomanda, 70000), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::Rivela, 75000), nullptr));
    CHECK(m.stato() == S::Rivelata);
    CHECK(m.timer().stato() == TimerGioco::Stato::Fermo);
    CHECK(m.timer().residuo_ms() == kTotale - 5000);
    CHECK_FALSE(m.prossima_scadenza().has_value());

    CHECK(m.gestisci(ev(TipoEvento::ProssimaDomanda, 80000), nullptr));
    CHECK(m.stato() == S::Finita);
    CHECK(m.giocate() == 2);
    CHECK_FALSE(m.gestisci(ev(TipoEvento::MostraDomanda, 90000), nullptr));
}

TEST_CASE("ESA: risposta giusta ferma il timer") {
    Esa m(frasi(), kLettura, kRisposta);
    REQUIRE(m.gestisci(ev(TipoEvento::MostraDomanda, 0), nullptr));
    CHECK(m.gestisci(ev(TipoEvento::ConfermaGiusta, 12000), nullptr));
    CHECK(m.stato() == S::Indovinata);
    CHECK(m.timer().residuo_ms() == kTotale - 12000);
    CHECK_FALSE(m.gestisci(ev(TipoEvento::TimerScaduto, kTotale), nullptr));
}

TEST_CASE("ESA senza frasi: subito Finita") {
    Esa m({}, kLettura, kRisposta);
    CHECK(m.stato() == S::Finita);
    CHECK_FALSE(m.gestisci(ev(TipoEvento::MostraDomanda), nullptr));
}

TEST_CASE("serata: la pausa generale congela il timer ESA e blocca la scadenza") {
    Partita p = in_gioco_esa();
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda, 0)));
    CHECK(*p.serata().prossima_scadenza() == kTotale);

    REQUIRE(p.applica(ev(TipoEvento::PausaGenerale, 5000)));
    CHECK_FALSE(p.serata().prossima_scadenza().has_value());
    CHECK_FALSE(p.applica(ev(TipoEvento::TimerScaduto, kTotale + 1)));  // in pausa non scade

    REQUIRE(p.applica(ev(TipoEvento::RiprendiGenerale, 20000)));
    CHECK(*p.serata().prossima_scadenza() == 20000 + (kTotale - 5000));
    CHECK(p.applica(ev(TipoEvento::TimerScaduto, 20000 + kTotale - 5000)));
    CHECK(esa(p).stato() == S::Scaduta);
}

TEST_CASE("annulla con un timer che corre: pausa generale, timer fermo al momento annullato") {
    Partita p = in_gioco_esa();
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda, 0)));
    REQUIRE(p.applica(ev(TipoEvento::Rivela, 10000)));  // clic sbagliato

    REQUIRE(p.applica(ev(TipoEvento::Annulla, 15000)));
    CHECK(esa(p).stato() == S::InGioco);
    CHECK(p.serata().in_pausa());
    CHECK(esa(p).timer().stato() == TimerGioco::Stato::InPausa);
    CHECK(esa(p).timer().residuo_ms() == kTotale - 10000);
    CHECK(p.registro().back().tipo == TipoEvento::PausaGenerale);
    CHECK(p.registro().back().arg == kPausaAutomatica);

    // un secondo annulla salta la pausa automatica e toglie MOSTRA_DOMANDA
    REQUIRE(p.applica(ev(TipoEvento::Annulla, 16000)));
    CHECK(esa(p).stato() == S::AttesaFrase);
    CHECK_FALSE(p.serata().in_pausa());
    CHECK(p.registro().back().tipo == TipoEvento::ScegliModalita);
}

TEST_CASE("annulla con timer: rigiocando gli stessi eventi si ottiene lo stesso stato") {
    const std::vector<Evento> eventi = {
        ev(TipoEvento::MostraDomanda, 0),      ev(TipoEvento::Rivela, 10000),
        ev(TipoEvento::Annulla, 15000),        ev(TipoEvento::RiprendiGenerale, 30000),
        ev(TipoEvento::ConfermaGiusta, 31000), ev(TipoEvento::Annulla, 32000),
    };
    Partita a = in_gioco_esa();
    Partita b = in_gioco_esa();
    for (const Evento& e : eventi) CHECK(a.applica(e));
    for (const Evento& e : eventi) b.applica(e);

    CHECK(esa(a).stato() == S::InGioco);
    CHECK(a.serata().in_pausa());
    // dopo la ripresa il timer ha corso 1 s prima della risposta annullata
    CHECK(esa(a).timer().residuo_ms() == kTotale - 10000 - 1000);
    CHECK(esa(b).timer().residuo_ms() == esa(a).timer().residuo_ms());
    CHECK(a.registro().size() == b.registro().size());
}

TEST_CASE("vista pubblico ESA: niente frase prima della rivelazione") {
    Partita p = in_gioco_esa();
    VistaPubblico v = vista_pubblico(p);
    REQUIRE(v.esa.has_value());
    CHECK(v.esa->stato == "AttesaFrase");
    CHECK(v.esa->indizi.empty());  // in attesa il pubblico non ha nulla della frase

    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda, 0)));
    v = vista_pubblico(p);
    CHECK(v.esa->indizi.size() == 3);
    CHECK(v.esa->frase.empty());
    CHECK(v.esa->timer.stato == "corre");
    CHECK(v.esa->timer.scadenza_ms == kTotale);
    CHECK(v.esa->timer.durata_ms == kRisposta);
    CHECK(v.esa->lettura_indizio_ms == kLettura);

    const VistaPlancia vp = vista_plancia(p);
    REQUIRE(vp.esa.has_value());
    REQUIRE(vp.esa->frase.has_value());
    CHECK(vp.esa->frase->soluzione == "Il dado è tratto");  // la plancia la vede subito
    CHECK(vp.contenuti.esa_frasi == 2);

    REQUIRE(p.applica(ev(TipoEvento::Rivela, 5000)));
    v = vista_pubblico(p);
    CHECK(v.esa->frase == "Il dado è tratto");
    CHECK(v.esa->timer.stato == "fermo");
}

TEST_CASE("TIMER_SCADUTO non è mai tra gli eventi ammessi della plancia") {
    Partita p = in_gioco_esa();
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda, 0)));
    for (const EventoAmmesso& a : eventi_ammessi(p)) CHECK(a.tipo != TipoEvento::TimerScaduto);
}
