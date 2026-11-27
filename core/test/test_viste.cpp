#include <doctest/doctest.h>

#include <algorithm>

#include "lascossa/viste.hpp"

using namespace lascossa;

namespace {
Contenuti contenuti() {
    Contenuti c;
    c.estas = {
        {"Piatti tipici regionali", {{"Arancini", true}, {"Sushi", false}}},
        {"Fiumi italiani", {{"Po", true}}},
    };
    return c;
}
Evento ev(TipoEvento t, int arg = -1) { return {t, arg}; }
const int kEstas = static_cast<int>(IdModalita::Estas);

bool ammesso(const std::vector<EventoAmmesso>& a, TipoEvento t, int arg = -1) {
    return std::any_of(a.begin(), a.end(), [&](const EventoAmmesso& x) { return x.tipo == t && x.arg == arg; });
}

Partita in_gioco_estas() {
    Partita p({3}, contenuti());
    REQUIRE(p.applica(ev(TipoEvento::AvviaSerata)));
    REQUIRE(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    return p;
}
}  // namespace

TEST_CASE("elenchi completi di eventi e modalità, conversione da nome") {
    REQUIRE(kTuttiGliEventi.size() == static_cast<std::size_t>(TipoEvento::Annulla) + 1);
    for (std::size_t i = 0; i < kTuttiGliEventi.size(); ++i) {
        CHECK(static_cast<std::size_t>(kTuttiGliEventi[i]) == i);
        CHECK(tipo_da_nome(nome(kTuttiGliEventi[i])) == kTuttiGliEventi[i]);
    }
    REQUIRE(kTutteLeModalita.size() == static_cast<std::size_t>(IdModalita::Duelli) + 1);
    for (std::size_t i = 0; i < kTutteLeModalita.size(); ++i) {
        CHECK(static_cast<std::size_t>(kTutteLeModalita[i]) == i);
    }
    CHECK_FALSE(tipo_da_nome("NON_ESISTE").has_value());
}

TEST_CASE("la copia della serata è profonda: provare un evento non tocca l'originale") {
    Partita p = in_gioco_estas();
    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));
    Serata copia(p.serata());
    CHECK(copia.gestisci(ev(TipoEvento::Seleziona, Estas::kEstas), nullptr));
    const auto* originale = dynamic_cast<const Estas*>(p.serata().modalita());
    REQUIRE(originale != nullptr);
    CHECK(originale->domanda()->stato() == CicloDomanda::Stato::Mostrata);
}

TEST_CASE("eventi ammessi seguono lo stato e non modificano la partita") {
    Partita p({3}, contenuti());
    auto a = eventi_ammessi(p);
    CHECK(ammesso(a, TipoEvento::AvviaSerata));
    CHECK_FALSE(ammesso(a, TipoEvento::ScegliModalita, kEstas));
    CHECK_FALSE(ammesso(a, TipoEvento::Annulla));  // registro vuoto

    REQUIRE(p.applica(ev(TipoEvento::AvviaSerata)));
    a = eventi_ammessi(p);
    CHECK(ammesso(a, TipoEvento::ScegliModalita, kEstas));
    CHECK_FALSE(ammesso(a, TipoEvento::ScegliModalita, static_cast<int>(IdModalita::Duelli)));
    CHECK(ammesso(a, TipoEvento::ChiudiSerata));
    CHECK(ammesso(a, TipoEvento::Annulla));

    REQUIRE(p.applica(ev(TipoEvento::ScegliModalita, kEstas)));
    a = eventi_ammessi(p);
    CHECK(ammesso(a, TipoEvento::ScegliCategoria, 0));
    CHECK(ammesso(a, TipoEvento::ScegliCategoria, 1));
    CHECK_FALSE(ammesso(a, TipoEvento::Seleziona, 0));

    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    a = eventi_ammessi(p);  // attesa: solo la categoria a schermo
    CHECK(ammesso(a, TipoEvento::MostraDomanda));
    CHECK(ammesso(a, TipoEvento::ScegliCategoria, 1));
    CHECK_FALSE(ammesso(a, TipoEvento::ScegliCategoria, 0));
    CHECK(ammesso(a, TipoEvento::PassaTurno));
    CHECK_FALSE(ammesso(a, TipoEvento::Seleziona, Estas::kEstas));
    CHECK_FALSE(ammesso(a, TipoEvento::Rivela));

    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));
    a = eventi_ammessi(p);
    CHECK_FALSE(ammesso(a, TipoEvento::MostraDomanda));
    CHECK(ammesso(a, TipoEvento::Seleziona, Estas::kEstas));
    CHECK(ammesso(a, TipoEvento::Seleziona, Estas::kNonEstas));
    CHECK(ammesso(a, TipoEvento::Rivela));
    CHECK_FALSE(ammesso(a, TipoEvento::Conferma));
    CHECK_FALSE(ammesso(a, TipoEvento::ProssimaDomanda));
    CHECK_FALSE(ammesso(a, TipoEvento::ScegliCategoria, 1));  // oggetto senza esito

    REQUIRE(p.applica(ev(TipoEvento::Seleziona, Estas::kEstas)));
    a = eventi_ammessi(p);
    CHECK(ammesso(a, TipoEvento::Conferma));

    const auto lunghezza = p.registro().size();
    (void)eventi_ammessi(p);
    CHECK(p.registro().size() == lunghezza);
    CHECK(p.traccia().size() == lunghezza);
}

TEST_CASE("pubblico: schermata di attesa fuori dal gioco") {
    Partita p({3}, contenuti());
    VistaPubblico v = vista_pubblico(p);
    CHECK_FALSE(v.modalita.has_value());
    CHECK_FALSE(v.estas.has_value());
    REQUIRE(p.applica(ev(TipoEvento::BlackoutOn)));
    CHECK(vista_pubblico(p).blackout);
}

TEST_CASE("pubblico: giallo, poi rosso sulla risposta sbagliata, senza mai mostrare la soluzione") {
    Partita p = in_gioco_estas();
    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));  // Arancini: Estas

    VistaPubblico v = vista_pubblico(p);
    CHECK(v.estas->stato == "OggettoInGioco");
    REQUIRE(v.estas.has_value());
    CHECK(v.estas->categoria == "Piatti tipici regionali");
    CHECK(v.estas->oggetto == "Arancini");
    REQUIRE(v.estas->opzioni.size() == 2);
    CHECK(v.estas->opzioni[0].stato == StatoOpzione::Neutra);
    CHECK(v.estas->opzioni[1].stato == StatoOpzione::Neutra);

    REQUIRE(p.applica(ev(TipoEvento::Seleziona, Estas::kNonEstas)));
    v = vista_pubblico(p);
    CHECK(v.estas->opzioni[0].stato == StatoOpzione::Neutra);
    CHECK(v.estas->opzioni[1].stato == StatoOpzione::Selezionata);

    REQUIRE(p.applica(ev(TipoEvento::Conferma)));
    v = vista_pubblico(p);
    CHECK(v.estas->opzioni[0].stato == StatoOpzione::Neutra);  // la giusta resta nascosta
    CHECK(v.estas->opzioni[1].stato == StatoOpzione::Sbagliata);
}

TEST_CASE("pubblico: verde sulla giusta, soluzione solo dopo Rivela") {
    Partita p = in_gioco_estas();
    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));
    REQUIRE(p.applica(ev(TipoEvento::Seleziona, Estas::kEstas)));
    REQUIRE(p.applica(ev(TipoEvento::Conferma)));
    CHECK(vista_pubblico(p).estas->opzioni[0].stato == StatoOpzione::Giusta);

    REQUIRE(p.applica(ev(TipoEvento::ProssimaDomanda)));
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));  // Sushi: Non Estas
    REQUIRE(p.applica(ev(TipoEvento::Rivela)));
    const VistaPubblico v = vista_pubblico(p);
    CHECK(v.estas->opzioni[0].stato == StatoOpzione::Neutra);
    CHECK(v.estas->opzioni[1].stato == StatoOpzione::Soluzione);
}

TEST_CASE("attesa: il pubblico vede solo la categoria, la plancia anche l'oggetto in arrivo") {
    Partita p = in_gioco_estas();
    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));

    const VistaPubblico pub = vista_pubblico(p);
    REQUIRE(pub.estas.has_value());
    CHECK(pub.estas->stato == "AttesaOggetto");
    CHECK(pub.estas->categoria == "Piatti tipici regionali");
    CHECK(pub.estas->oggetto.empty());
    CHECK(pub.estas->opzioni.empty());

    const VistaPlancia pl = vista_plancia(p);
    REQUIRE(pl.estas.has_value());
    CHECK(pl.estas->stato == "AttesaOggetto");
    CHECK_FALSE(pl.estas->oggetto.has_value());
    CHECK(pl.estas->anteprima == "Arancini");
    CHECK(pl.estas->anteprima_appartiene == true);
    CHECK(pl.estas->categorie[0].giocati == 0);

    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));
    CHECK_FALSE(vista_plancia(p).estas->anteprima.has_value());
    CHECK(vista_pubblico(p).estas->oggetto == "Arancini");
}

TEST_CASE("plancia: soluzione, categorie, turno e debug") {
    Partita p = in_gioco_estas();
    REQUIRE(p.applica(ev(TipoEvento::ScegliCategoria, 0)));
    REQUIRE(p.applica(ev(TipoEvento::MostraDomanda)));
    CHECK_FALSE(p.applica(ev(TipoEvento::Conferma)));  // rifiutato: niente selezione

    const VistaPlancia v = vista_plancia(p);
    CHECK(v.stato_serata == "InGioco");
    CHECK(v.numero_concorrenti == 3);
    CHECK(v.contenuti.estas_categorie == 2);
    CHECK(v.contenuti.estas_oggetti == 3);
    REQUIRE(v.modalita_attiva.has_value());
    CHECK(*v.modalita_attiva == IdModalita::Estas);
    CHECK(v.modalita[0].disponibile);
    CHECK(v.modalita[0].giocata);
    CHECK_FALSE(v.modalita[7].disponibile);

    REQUIRE(v.estas.has_value());
    CHECK(v.estas->stato == "OggettoInGioco");
    CHECK(v.estas->oggetto == "Arancini");
    CHECK(v.estas->oggetto_appartiene == true);
    CHECK(v.estas->stato_domanda == "Mostrata");
    REQUIRE(v.estas->opzioni.size() == 2);
    CHECK(v.estas->opzioni[0].corretta);
    CHECK_FALSE(v.estas->opzioni[1].corretta);
    CHECK(v.estas->categorie[0].giocati == 1);
    CHECK(v.estas->categorie[0].totale == 2);
    CHECK(v.estas->numero_concorrenti == 3);

    CHECK(v.lunghezza_registro == 4);
    REQUIRE(v.ultimi_eventi.size() == 4);
    CHECK(v.ultimi_eventi.back().seq == 4);
    REQUIRE(v.ultimi_rifiutati.size() == 1);
    CHECK(v.ultimi_rifiutati[0].evento.tipo == TipoEvento::Conferma);
}
