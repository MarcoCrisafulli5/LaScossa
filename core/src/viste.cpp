#include "lascossa/viste.hpp"

#include <algorithm>
#include <array>

#include "lascossa/estas.hpp"

namespace lascossa {

namespace {

constexpr std::array<std::string_view, 2> kEtichetteEstas = {"Estas", "Non Estas"};

// Stato grafico di ogni opzione di una domanda, uguale per plancia e pubblico.
StatoOpzione stato_opzione(const CicloDomanda& d, int i) {
    if (d.bruciata(i)) {
        return StatoOpzione::Sbagliata;
    }
    switch (d.stato()) {
        case CicloDomanda::Stato::Mostrata: return StatoOpzione::Neutra;
        case CicloDomanda::Stato::Selezionata:
            return i == d.selezionata() ? StatoOpzione::Selezionata : StatoOpzione::Neutra;
        case CicloDomanda::Stato::Giusta:
            return i == d.selezionata() ? StatoOpzione::Giusta : StatoOpzione::Neutra;
        case CicloDomanda::Stato::Sbagliata: return StatoOpzione::Neutra;  // la sbagliata è già bruciata
        case CicloDomanda::Stato::Rivelata:
            return i == d.corretta() ? StatoOpzione::Soluzione : StatoOpzione::Neutra;
    }
    return StatoOpzione::Neutra;
}

const Estas* come_estas(const Modalita* m) {
    return m != nullptr && m->id() == IdModalita::Estas ? dynamic_cast<const Estas*>(m) : nullptr;
}

// ------------------------------------------------------------ Estas

EstasPubblico estas_pubblico(const Estas& m) {
    EstasPubblico v;
    if (m.categoria() >= 0) {
        v.categoria = m.categorie()[static_cast<std::size_t>(m.categoria())].nome;
    }
    const OggettoEstas* o = m.oggetto_corrente();
    const CicloDomanda* d = m.domanda();
    if (o != nullptr && d != nullptr) {
        v.oggetto = o->nome;
        for (int i = 0; i < d->num_opzioni(); ++i) {
            v.opzioni.push_back({std::string(kEtichetteEstas[static_cast<std::size_t>(i)]), stato_opzione(*d, i)});
        }
    }
    return v;
}

EstasPlancia estas_plancia(const Estas& m) {
    EstasPlancia v;
    v.stato = std::string(m.nome_stato());
    for (std::size_t i = 0; i < m.categorie().size(); ++i) {
        const CategoriaEstas& c = m.categorie()[i];
        v.categorie.push_back({c.nome, c.oggetti.size(), m.giocati(static_cast<int>(i))});
    }
    v.categoria_corrente = m.categoria();
    v.concorrente_di_turno = m.concorrente_di_turno();
    v.numero_concorrenti = m.numero_concorrenti();
    const OggettoEstas* o = m.oggetto_corrente();
    const CicloDomanda* d = m.domanda();
    if (o != nullptr && d != nullptr) {
        v.oggetto = o->nome;
        v.oggetto_appartiene = o->appartiene;
        v.stato_domanda = std::string(CicloDomanda::nome(d->stato()));
        v.domanda_conclusa = d->conclusa();
        for (int i = 0; i < d->num_opzioni(); ++i) {
            v.opzioni.push_back(
                {std::string(kEtichetteEstas[static_cast<std::size_t>(i)]), stato_opzione(*d, i), i == d->corretta()});
        }
    }
    return v;
}

// Argomenti da provare per gli eventi che ne hanno uno, in base alla modalità attiva.
std::vector<int> argomenti(TipoEvento t, const Serata& s) {
    std::vector<int> args;
    const Estas* estas = come_estas(s.modalita());
    switch (t) {
        case TipoEvento::ScegliModalita:
            for (IdModalita m : kTutteLeModalita) args.push_back(static_cast<int>(m));
            break;
        case TipoEvento::ScegliCategoria:
            if (estas != nullptr) {
                for (std::size_t i = 0; i < estas->categorie().size(); ++i) args.push_back(static_cast<int>(i));
            }
            break;
        case TipoEvento::Seleziona:
            if (estas != nullptr && estas->domanda() != nullptr) {
                for (int i = 0; i < estas->domanda()->num_opzioni(); ++i) args.push_back(i);
            }
            break;
        case TipoEvento::AvviaSerata:
        case TipoEvento::FineModalita:
        case TipoEvento::ChiudiSerata:
        case TipoEvento::PausaGenerale:
        case TipoEvento::RiprendiGenerale:
        case TipoEvento::BlackoutOn:
        case TipoEvento::BlackoutOff:
        case TipoEvento::MostraRiquadro:
        case TipoEvento::NascondiRiquadro:
        case TipoEvento::AggiornaRiquadro:
        case TipoEvento::RicaricaContenuti:
        case TipoEvento::ProssimaDomanda:
        case TipoEvento::Conferma:
        case TipoEvento::ConfermaGiusta:
        case TipoEvento::ConfermaSbagliata:
        case TipoEvento::Rivela:
        case TipoEvento::PassaTurno:
        case TipoEvento::Annulla: args.push_back(-1); break;
    }
    return args;
}

template <typename T>
std::vector<T> ultimi(const std::vector<T>& v, std::size_t n) {
    const std::size_t da = v.size() > n ? v.size() - n : 0;
    return std::vector<T>(v.begin() + static_cast<std::ptrdiff_t>(da), v.end());
}

}  // namespace

std::string_view nome(StatoOpzione s) {
    switch (s) {
        case StatoOpzione::Neutra: return "neutra";
        case StatoOpzione::Selezionata: return "selezionata";
        case StatoOpzione::Giusta: return "giusta";
        case StatoOpzione::Sbagliata: return "sbagliata";
        case StatoOpzione::Soluzione: return "soluzione";
    }
    return "?";
}

std::vector<EventoAmmesso> eventi_ammessi(const Partita& p) {
    std::vector<EventoAmmesso> ammessi;
    for (TipoEvento t : kTuttiGliEventi) {
        if (t == TipoEvento::Annulla) {
            if (!p.registro().empty()) ammessi.push_back({t, -1});
            continue;
        }
        for (int arg : argomenti(t, p.serata())) {
            Serata prova(p.serata());
            if (prova.gestisci(Evento{t, arg}, nullptr)) {
                ammessi.push_back({t, arg});
            }
        }
    }
    return ammessi;
}

VistaPubblico vista_pubblico(const Partita& p) {
    const Serata& s = p.serata();
    VistaPubblico v;
    v.blackout = s.blackout();
    v.riquadro_visibile = s.riquadro_visibile();
    v.riquadro_testo = s.riquadro_testo();
    if (s.stato() == Serata::Stato::InGioco && s.modalita() != nullptr) {
        v.modalita = s.modalita()->id();
        if (const Estas* e = come_estas(s.modalita())) {
            v.estas = estas_pubblico(*e);
        }
    }
    return v;
}

VistaPlancia vista_plancia(const Partita& p, std::size_t quanti_debug) {
    const Serata& s = p.serata();
    VistaPlancia v;
    v.stato_serata = std::string(Serata::nome(s.stato()));
    v.pausa = s.in_pausa();
    v.blackout = s.blackout();
    v.riquadro_visibile = s.riquadro_visibile();
    v.riquadro_testo = s.riquadro_testo();
    v.numero_concorrenti = s.configurazione().numero_concorrenti;

    v.contenuti.estas_categorie = s.contenuti().estas.size();
    for (const CategoriaEstas& c : s.contenuti().estas) v.contenuti.estas_oggetti += c.oggetti.size();

    for (IdModalita m : kTutteLeModalita) {
        const bool giocata = std::find(s.giocate().begin(), s.giocate().end(), m) != s.giocate().end();
        v.modalita.push_back({m, Serata::disponibile(m), giocata});
    }
    if (s.modalita() != nullptr) {
        v.modalita_attiva = s.modalita()->id();
        v.stato_modalita = std::string(s.modalita()->nome_stato());
        if (const Estas* e = come_estas(s.modalita())) {
            v.estas = estas_plancia(*e);
        }
    }
    v.ammessi = eventi_ammessi(p);

    v.lunghezza_registro = p.registro().size();
    const std::size_t da = p.registro().size() > quanti_debug ? p.registro().size() - quanti_debug : 0;
    for (std::size_t i = da; i < p.registro().size(); ++i) {
        v.ultimi_eventi.push_back({i + 1, p.registro()[i]});
    }
    v.ultimi_rifiutati = ultimi(p.rifiutati(), quanti_debug);
    v.ultime_transizioni = ultimi(p.traccia(), quanti_debug);
    return v;
}

}  // namespace lascossa
