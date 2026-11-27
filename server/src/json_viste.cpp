#include "json_viste.hpp"

#include <string>

namespace lascossa::server {

using nlohmann::json;

namespace {

std::string s(std::string_view v) { return std::string(v); }

json modalita(IdModalita m) { return {{"id", static_cast<int>(m)}, {"nome", s(nome(m))}}; }

json evento(const Evento& e) {
    return {{"evento", s(nome(e.tipo))}, {"arg", e.arg}, {"testo", e.testo}, {"t", e.t_ms}};
}

json estas(const EstasPlancia& e) {
    json categorie = json::array();
    for (const auto& c : e.categorie) {
        categorie.push_back({{"nome", c.nome}, {"totale", c.totale}, {"giocati", c.giocati}});
    }
    json opzioni = json::array();
    for (const auto& o : e.opzioni) {
        opzioni.push_back({{"etichetta", o.etichetta}, {"stato", s(nome(o.stato))}, {"corretta", o.corretta}});
    }
    return {
        {"stato", e.stato},
        {"categorie", categorie},
        {"categoriaCorrente", e.categoria_corrente},
        {"oggetto", e.oggetto ? json(*e.oggetto) : json(nullptr)},
        {"oggettoAppartiene", e.oggetto_appartiene ? json(*e.oggetto_appartiene) : json(nullptr)},
        {"anteprima", e.anteprima ? json(*e.anteprima) : json(nullptr)},
        {"anteprimaAppartiene", e.anteprima_appartiene ? json(*e.anteprima_appartiene) : json(nullptr)},
        {"statoDomanda", e.stato_domanda},
        {"domandaConclusa", e.domanda_conclusa},
        {"opzioni", opzioni},
        {"concorrenteDiTurno", e.concorrente_di_turno},
        {"numeroConcorrenti", e.numero_concorrenti},
        {"attesaEsitoMs", e.attesa_esito_ms},
    };
}

json estas(const EstasPubblico& e) {
    json opzioni = json::array();
    for (const auto& o : e.opzioni) {
        opzioni.push_back({{"etichetta", o.etichetta}, {"stato", s(nome(o.stato))}});
    }
    return {{"stato", e.stato},
            {"categoria", e.categoria},
            {"oggetto", e.oggetto},
            {"opzioni", opzioni},
            {"attesaEsitoMs", e.attesa_esito_ms}};
}

json timer(const TimerVista& t) {
    return {{"stato", t.stato},
            {"durataMs", t.durata_ms},
            {"totaleMs", t.totale_ms},
            {"scadenzaMs", t.scadenza_ms},
            {"residuoMs", t.residuo_ms}};
}

json frase(const FrasePlancia& f) { return {{"indizi", f.indizi}, {"soluzione", f.soluzione}}; }

json esa(const EsaPlancia& e) {
    return {
        {"stato", e.stato},
        {"giocate", e.giocate},
        {"totale", e.totale},
        {"frase", e.frase ? frase(*e.frase) : json(nullptr)},
        {"anteprima", e.anteprima ? frase(*e.anteprima) : json(nullptr)},
        {"timer", timer(e.timer)},
        {"letturaIndizioMs", e.lettura_indizio_ms},
    };
}

json esa(const EsaPubblico& e) {
    return {
        {"stato", e.stato},
        {"indizi", e.indizi},
        {"frase", e.frase},
        {"timer", timer(e.timer)},
        {"letturaIndizioMs", e.lettura_indizio_ms},
    };
}

}  // namespace

json a_json(const VistaPlancia& v) {
    json mod = json::array();
    for (const auto& m : v.modalita) {
        json x = modalita(m.id);
        x["disponibile"] = m.disponibile;
        x["giocata"] = m.giocata;
        mod.push_back(std::move(x));
    }
    json ammessi = json::array();
    for (const auto& a : v.ammessi) ammessi.push_back({{"evento", s(nome(a.tipo))}, {"arg", a.arg}});

    json eventi = json::array();
    for (const auto& e : v.ultimi_eventi) {
        json x = evento(e.evento);
        x["seq"] = e.seq;
        eventi.push_back(std::move(x));
    }
    json rifiutati = json::array();
    for (const auto& r : v.ultimi_rifiutati) {
        json x = evento(r.evento);
        x["motivo"] = r.motivo;
        rifiutati.push_back(std::move(x));
    }
    json transizioni = json::array();
    for (const auto& t : v.ultime_transizioni) {
        transizioni.push_back({{"macchina", s(t.macchina)}, {"da", s(t.da)}, {"evento", s(nome(t.evento))}, {"a", s(t.a)}});
    }

    return {
        {"statoSerata", v.stato_serata},
        {"pausa", v.pausa},
        {"blackout", v.blackout},
        {"riquadro", {{"visibile", v.riquadro_visibile}, {"testo", v.riquadro_testo}}},
        {"numeroConcorrenti", v.numero_concorrenti},
        {"contenuti",
         {{"estasCategorie", v.contenuti.estas_categorie},
          {"estasOggetti", v.contenuti.estas_oggetti},
          {"esaFrasi", v.contenuti.esa_frasi}}},
        {"modalita", mod},
        {"modalitaAttiva", v.modalita_attiva ? modalita(*v.modalita_attiva) : json(nullptr)},
        {"statoModalita", v.stato_modalita},
        {"estas", v.estas ? estas(*v.estas) : json(nullptr)},
        {"esa", v.esa ? esa(*v.esa) : json(nullptr)},
        {"ammessi", ammessi},
        {"debug",
         {{"lunghezzaRegistro", v.lunghezza_registro},
          {"ultimiEventi", eventi},
          {"ultimiRifiutati", rifiutati},
          {"ultimeTransizioni", transizioni}}},
    };
}

json a_json(const VistaPubblico& v) {
    return {
        {"blackout", v.blackout},
        {"riquadro", {{"visibile", v.riquadro_visibile}, {"testo", v.riquadro_testo}}},
        {"modalita", v.modalita ? modalita(*v.modalita) : json(nullptr)},
        {"estas", v.estas ? estas(*v.estas) : json(nullptr)},
        {"esa", v.esa ? esa(*v.esa) : json(nullptr)},
    };
}

}  // namespace lascossa::server
