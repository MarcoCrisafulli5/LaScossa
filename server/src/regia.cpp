#include "regia.hpp"

#include <chrono>

#include "json_viste.hpp"
#include "lascossa/viste.hpp"

namespace lascossa::server {

namespace fs = std::filesystem;
using nlohmann::json;

namespace {

constexpr std::size_t kIdRicordati = 256;
constexpr std::size_t kMassimoTestoRiquadro = 500;

std::string testo(const json& j) { return j.dump(); }

std::string ack(const std::string& id) { return testo({{"tipo", "ack"}, {"id", id}}); }

std::string reject(const json& id, const std::string& motivo) {
    return testo({{"tipo", "reject"}, {"id", id}, {"motivo", motivo}});
}

}  // namespace

Regia::Regia(fs::path cartella_dati, bool nuova) : cartella_dati_(std::move(cartella_dati)) {
    apri_partita(carica_dati(cartella_dati_), nuova);
}

void Regia::apri_partita(DatiSerata dati, bool nuova) {
    impostazioni_ = dati.impostazioni;
    avvisi_ = std::move(dati.avvisi);
    partita_ = std::make_unique<Partita>(dati.configurazione, std::move(dati.contenuti));
    const fs::path cartella_partite = cartella_dati_ / "partite";
    const auto ora = std::chrono::system_clock::now();

    const auto ultimo = nuova ? std::nullopt : RegistroSuFile::ultimo(cartella_partite);
    if (!ultimo) {
        registro_.emplace(RegistroSuFile::nuovo(cartella_partite, ora));
        return;
    }

    LetturaRegistro letti = RegistroSuFile::leggi(*ultimo);
    if (letti.avviso) avvisi_.push_back(*letti.avviso);
    std::size_t rifiutati = 0;
    for (const Evento& e : letti.eventi) {
        if (!partita_->applica(e)) ++rifiutati;
    }
    if (rifiutati > 0) {
        avvisi_.push_back(std::to_string(rifiutati) + " eventi del registro " + ultimo->filename().generic_string() +
                          " rifiutati durante la ripresa (contenuti cambiati?)");
    }

    if (partita_->serata().stato() == Serata::Stato::Chiusa) {
        // La partita precedente era finita: se ne apre una nuova con gli stessi dati.
        auto cfg = partita_->serata().configurazione();
        auto contenuti = partita_->serata().contenuti();
        partita_ = std::make_unique<Partita>(cfg, std::move(contenuti));
        registro_.emplace(RegistroSuFile::nuovo(cartella_partite, ora));
        return;
    }
    if (!letti.eventi.empty()) {
        avvisi_.push_back("ripresa la partita " + ultimo->filename().generic_string() + " (" +
                          std::to_string(letti.eventi.size()) + " eventi)");
    }
    registro_.emplace(RegistroSuFile::riprendi(*ultimo, letti.eventi.size()));

    // Un timer di gioco che correva al momento del crash scadrebbe subito per il tempo passato:
    // si riprende in pausa generale, con il timer fermo all'ultimo evento registrato.
    if (partita_->serata().prossima_scadenza() && !letti.eventi.empty()) {
        const Evento pausa{TipoEvento::PausaGenerale, kPausaAutomatica, letti.eventi.back().t_ms};
        const std::string da = stato_corrente();
        if (partita_->applica(pausa)) {
            registro_->aggiungi(pausa, da);
            avvisi_.push_back("un timer di gioco correva: la partita riprende in pausa generale");
        }
    }
}

std::vector<Uscita> Regia::scadenza(std::int64_t ora) {
    const auto quando = prossima_scadenza();
    if (!quando || ora < *quando) return {};
    const Evento e{TipoEvento::TimerScaduto, -1, ora};
    const std::string da = stato_corrente();
    if (!partita_->applica(e)) return {};
    registro_->aggiungi(e, da);  // prima il registro, poi gli schermi
    return a_tutti(ora);
}

void Regia::connesso(IdClient c) { client_[c] = Ruolo::Nessuno; }

void Regia::disconnesso(IdClient c) { client_.erase(c); }

std::vector<Uscita> Regia::messaggio(IdClient c, std::string_view testo_msg, std::int64_t ora) {
    json m;
    try {
        m = json::parse(testo_msg);
    } catch (const json::parse_error&) {
        return {{c, reject(nullptr, "messaggio non valido: JSON malformato")}};
    }
    const std::string tipo = m.is_object() ? m.value("tipo", std::string()) : std::string();
    if (tipo == "hello") return su_hello(c, m, ora);
    if (tipo == "ping") return {{c, testo({{"tipo", "pong"}, {"oraServer", ora}})}};
    if (tipo == "event") return su_evento(c, m, ora);
    return {{c, reject(nullptr, "tipo di messaggio sconosciuto: \"" + tipo + "\"")}};
}

std::vector<Uscita> Regia::su_hello(IdClient c, const json& m, std::int64_t ora) {
    const std::string ruolo = m.value("ruolo", std::string());
    Ruolo r = Ruolo::Nessuno;
    if (ruolo == "plancia") {
        if (impostazioni_.pin) {
            const auto pin = m.contains("pin") && m["pin"].is_string() ? m["pin"].get<std::string>() : std::string();
            if (pin != *impostazioni_.pin) return {{c, reject(nullptr, "PIN della plancia errato")}};
        }
        r = Ruolo::Plancia;
    } else if (ruolo == "pubblico") {
        r = Ruolo::Pubblico;
    } else {
        return {{c, reject(nullptr, "ruolo sconosciuto: usa \"plancia\" o \"pubblico\"")}};
    }
    client_[c] = r;
    return {{c, messaggio_vista("snapshot", r, ora)}};
}

std::vector<Uscita> Regia::su_evento(IdClient c, const json& m, std::int64_t ora) {
    const json id_json = m.contains("id") ? m["id"] : json(nullptr);
    const auto ruolo = client_.find(c);
    if (ruolo == client_.end() || ruolo->second != Ruolo::Plancia) {
        return {{c, reject(id_json, "solo la plancia può inviare eventi")}};
    }
    if (!id_json.is_string() || id_json.get<std::string>().empty()) {
        return {{c, reject(id_json, "evento senza id")}};
    }
    const std::string id = id_json.get<std::string>();
    if (gia_visto(id)) return {{c, ack(id)}};  // doppio invio: già applicato, si conferma soltanto

    const std::string nome_evento = m.value("evento", std::string());
    if (nome_evento == kNuovaPartita) return su_nuova_partita(c, id, ora);

    const auto tipo = tipo_da_nome(nome_evento);
    if (!tipo) return {{c, reject(id, "evento sconosciuto: \"" + nome_evento + "\"")}};
    if (*tipo == TipoEvento::TimerScaduto) return {{c, reject(id, "TIMER_SCADUTO lo genera solo il server")}};

    Evento e{*tipo};
    e.arg = m.contains("arg") && m["arg"].is_number_integer() ? m["arg"].get<int>() : -1;
    if (*tipo == TipoEvento::PausaGenerale) e.arg = -1;  // kPausaAutomatica la mette solo il sistema
    e.t_ms = ora;
    e.testo = m.contains("testo") && m["testo"].is_string() ? m["testo"].get<std::string>() : std::string();
    if (e.testo.size() > kMassimoTestoRiquadro) {
        return {{c, reject(id, "testo troppo lungo (massimo " + std::to_string(kMassimoTestoRiquadro) + " caratteri)")}};
    }

    const std::string da = stato_corrente();
    const Esito esito = partita_->applica(e);
    ricorda(id);
    if (!esito) {
        // Il rifiuto cambia solo i dati di debug: si aggiorna la plancia con il motivo.
        std::vector<Uscita> out{{c, reject(id, esito.motivo)}};
        ++seq_;
        for (const auto& [altro, r] : client_) {
            if (r == Ruolo::Plancia) out.push_back({altro, messaggio_vista("update", r, ora)});
        }
        return out;
    }

    // Prima il registro su disco, poi gli schermi: dopo un crash lo stato è quello già visto.
    registro_->aggiungi(e, da);
    std::vector<Uscita> out{{c, ack(id)}};
    auto viste = a_tutti(ora);
    out.insert(out.end(), viste.begin(), viste.end());
    return out;
}

std::vector<Uscita> Regia::su_nuova_partita(IdClient c, const std::string& id, std::int64_t ora) {
    const auto stato = partita_->serata().stato();
    if (stato != Serata::Stato::Lobby && stato != Serata::Stato::Chiusa) {
        return {{c, reject(id, "nuova partita solo nella lobby o a serata chiusa")}};
    }
    DatiSerata dati;
    try {
        dati = carica_dati(cartella_dati_);
    } catch (const ErroreDati& e) {
        return {{c, reject(id, std::string("dati non validi, partita non cambiata: ") + e.what())}};
    }
    apri_partita(std::move(dati), /*nuova=*/true);
    ricorda(id);
    std::vector<Uscita> out{{c, ack(id)}};
    auto viste = a_tutti(ora);
    out.insert(out.end(), viste.begin(), viste.end());
    return out;
}

json Regia::vista(Ruolo r) const {
    if (r == Ruolo::Pubblico) return a_json(vista_pubblico(*partita_));
    json v = a_json(vista_plancia(*partita_));
    const auto stato = partita_->serata().stato();
    v["server"] = {
        {"registro", registro_->file().filename().generic_string()},
        {"avvisi", avvisi_},
        {"nuovaPartitaAmmessa", stato == Serata::Stato::Lobby || stato == Serata::Stato::Chiusa},
        {"pinAttivo", impostazioni_.pin.has_value()},
    };
    return v;
}

std::string Regia::messaggio_vista(std::string_view tipo, Ruolo r, std::int64_t ora) const {
    return testo({{"tipo", std::string(tipo)}, {"seq", seq_}, {"oraServer", ora}, {"vista", vista(r)}});
}

std::vector<Uscita> Regia::a_tutti(std::int64_t ora) {
    ++seq_;
    std::vector<Uscita> out;
    std::optional<std::string> plancia, pubblico;  // calcolate una volta sola per ruolo
    for (const auto& [c, r] : client_) {
        if (r == Ruolo::Plancia) {
            if (!plancia) plancia = messaggio_vista("update", r, ora);
            out.push_back({c, *plancia});
        } else if (r == Ruolo::Pubblico) {
            if (!pubblico) pubblico = messaggio_vista("update", r, ora);
            out.push_back({c, *pubblico});
        }
    }
    return out;
}

bool Regia::gia_visto(const std::string& id) const { return id_visti_.count(id) > 0; }

void Regia::ricorda(const std::string& id) {
    id_recenti_.push_back(id);
    id_visti_.insert(id);
    if (id_recenti_.size() > kIdRicordati) {
        id_visti_.erase(id_recenti_.front());
        id_recenti_.pop_front();
    }
}

std::string Regia::stato_corrente() const {
    const Serata& s = partita_->serata();
    std::string da(Serata::nome(s.stato()));
    if (s.modalita() != nullptr) da += "/" + std::string(s.modalita()->nome_stato());
    return da;
}

}  // namespace lascossa::server
