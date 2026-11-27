// Regia: il protocollo client-server senza la rete.
//
// Riceve i messaggi JSON dei client (identificati da un numero) e restituisce i messaggi da
// inviare. Possiede la Partita e il registro su file. Il codice di rete (main.cpp, Crow) si
// limita a consegnare i messaggi: così il protocollo si testa senza aprire socket.
//
// Messaggi (vedi documento di architettura, "Protocollo tra client e server"):
//   client -> server  hello {ruolo, pin?}   event {id, evento, arg?, testo?}   ping
//   server -> client  snapshot / update {seq, oraServer, vista}   ack {id} / reject {id, motivo}   pong
// Solo la plancia invia eventi. Il pubblico riceve solo la propria vista.
#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "dati.hpp"
#include "lascossa/partita.hpp"
#include "registro_file.hpp"

namespace lascossa::server {

using IdClient = std::uint64_t;

struct Uscita {
    IdClient destinatario;
    std::string testo;
};

// Comando gestito dal server fuori dalle macchine a stati: ricarica i dati e apre un registro nuovo.
// Ammesso solo nella lobby o a serata chiusa, per non perdere una partita con un clic sbagliato.
inline constexpr std::string_view kNuovaPartita = "NUOVA_PARTITA";

class Regia {
public:
    // Carica i dati e riprende l'ultima partita non chiusa (a meno che nuova = true).
    // Lancia ErroreDati se i dati non sono validi.
    Regia(std::filesystem::path cartella_dati, bool nuova);

    void connesso(IdClient c);
    void disconnesso(IdClient c);
    std::vector<Uscita> messaggio(IdClient c, std::string_view testo, std::int64_t ora_ms);

    // Timer di gioco. Il server richiama scadenza() quando arriva l'ora di prossima_scadenza():
    // se il timer è davvero scaduto, TIMER_SCADUTO entra come un evento (registro, poi viste a tutti).
    std::optional<std::int64_t> prossima_scadenza() const { return partita_->serata().prossima_scadenza(); }
    std::vector<Uscita> scadenza(std::int64_t ora_ms);

    const Partita& partita() const { return *partita_; }
    const std::vector<std::string>& avvisi() const { return avvisi_; }
    std::filesystem::path file_registro() const { return registro_->file(); }

private:
    enum class Ruolo { Nessuno, Plancia, Pubblico };

    void apri_partita(DatiSerata dati, bool nuova);
    std::vector<Uscita> su_hello(IdClient c, const nlohmann::json& m, std::int64_t ora);
    std::vector<Uscita> su_evento(IdClient c, const nlohmann::json& m, std::int64_t ora);
    std::vector<Uscita> su_nuova_partita(IdClient c, const std::string& id, std::int64_t ora);

    nlohmann::json vista(Ruolo r) const;
    std::string messaggio_vista(std::string_view tipo, Ruolo r, std::int64_t ora) const;
    std::vector<Uscita> a_tutti(std::int64_t ora);
    bool gia_visto(const std::string& id) const;
    void ricorda(const std::string& id);
    std::string stato_corrente() const;

    std::filesystem::path cartella_dati_;
    Impostazioni impostazioni_;
    std::unique_ptr<Partita> partita_;
    std::optional<RegistroSuFile> registro_;
    std::vector<std::string> avvisi_;
    std::map<IdClient, Ruolo> client_;
    std::uint64_t seq_ = 0;  // versione della vista, cresce a ogni aggiornamento
    std::deque<std::string> id_recenti_;
    std::unordered_set<std::string> id_visti_;
};

}  // namespace lascossa::server
