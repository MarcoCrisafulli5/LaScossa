// Eseguibile del server: HTTP per le pagine, WebSocket /ws per il gioco.
// Tutta la logica sta in Regia (protocollo) e nel core (macchine a stati): qui c'è solo la rete.
//
// Uso: lascossa-server [--dati DIR] [--web DIR] [--porta N] [--indirizzo IP] [--nuova]
//   --dati       cartella con config.json, contenuti/ e partite/   (default: dati)
//   --web        cartella con la build di React (web/dist)          (default: web/dist)
//   --porta      porta HTTP                                         (default: 8080)
//   --indirizzo  indirizzo su cui ascoltare                         (default: 127.0.0.1;
//                0.0.0.0 per tablet in Wi-Fi o dentro Docker)
//   --nuova      non riprendere l'ultima partita, aprine una nuova

#include <crow.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "csv.hpp"
#include "regia.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
using lascossa::server::IdClient;
using lascossa::server::Regia;

namespace {

struct Opzioni {
    fs::path dati = "dati";
    fs::path web = "web/dist";
    std::uint16_t porta = 8080;
    std::string indirizzo = "127.0.0.1";
    bool nuova = false;
};

std::optional<Opzioni> leggi_opzioni(int argc, char** argv) {
    Opzioni o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto valore = [&]() -> std::optional<std::string> {
            if (i + 1 >= argc) return std::nullopt;
            return std::string(argv[++i]);
        };
        if (a == "--nuova") {
            o.nuova = true;
        } else if (a == "--dati" || a == "--web" || a == "--porta" || a == "--indirizzo") {
            const auto v = valore();
            if (!v) return std::nullopt;
            if (a == "--dati") o.dati = *v;
            else if (a == "--web") o.web = *v;
            else if (a == "--indirizzo") o.indirizzo = *v;
            else o.porta = static_cast<std::uint16_t>(std::stoi(*v));
        } else {
            return std::nullopt;
        }
    }
    return o;
}

std::int64_t ora_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Serve un file della build web; le pagine dell'app (/plancia, /pubblico, /debug) ricevono index.html.
crow::response file_web(const fs::path& web, const std::string& percorso) {
    const fs::path relativo = fs::path(percorso).lexically_normal();
    if (relativo.is_absolute() || (!relativo.empty() && *relativo.begin() == "..")) {
        return crow::response(404);
    }
    fs::path p = web / relativo;
    if (percorso.empty() || !fs::is_regular_file(p)) {
        if (relativo.has_extension()) return crow::response(404);
        p = web / "index.html";
        if (!fs::is_regular_file(p)) {
            return crow::response(503, "Interfaccia non compilata: esegui \"npm run build\" in web/");
        }
    }
    crow::response r(200);
    r.body = lascossa::server::leggi_file(p);
    const std::string est = p.extension().string().empty() ? "" : p.extension().string().substr(1);
    r.set_header("Content-Type", crow::response::get_mime_type(est));
    if (p.filename() == "index.html") r.set_header("Cache-Control", "no-cache");
    return r;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const auto opzioni = leggi_opzioni(argc, argv);
    if (!opzioni) {
        std::cerr << "uso: lascossa-server [--dati DIR] [--web DIR] [--porta N] [--indirizzo IP] [--nuova]\n";
        return 2;
    }

    std::optional<Regia> regia;
    try {
        regia.emplace(opzioni->dati, opzioni->nuova);
    } catch (const lascossa::server::ErroreDati& e) {
        std::cerr << "Errore nei dati: " << e.what() << "\n";
        return 1;
    }

    std::cout << "La Scossa - server " << LASCOSSA_VERSIONE << "\n"
              << "  dati:     " << fs::absolute(opzioni->dati).generic_string() << "\n"
              << "  registro: " << regia->file_registro().generic_string() << "\n";
    for (const auto& a : regia->avvisi()) std::cout << "  avviso:   " << a << "\n";

    std::mutex mutex;  // un evento alla volta: run-to-completion, come nelle macchine a stati
    std::map<crow::websocket::connection*, IdClient> id_di;
    std::map<IdClient, crow::websocket::connection*> conn_di;
    IdClient prossimo_id = 1;

    const auto consegna = [&](const std::vector<lascossa::server::Uscita>& uscite) {
        for (const auto& u : uscite) {
            const auto it = conn_di.find(u.destinatario);
            if (it != conn_di.end()) it->second->send_text(u.testo);
        }
    };

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Warning);

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([&](crow::websocket::connection& conn) {
            std::lock_guard lock(mutex);
            const IdClient id = prossimo_id++;
            id_di[&conn] = id;
            conn_di[id] = &conn;
            regia->connesso(id);
        })
        .onclose([&](crow::websocket::connection& conn, const std::string&, std::uint16_t) {
            std::lock_guard lock(mutex);
            const auto it = id_di.find(&conn);
            if (it == id_di.end()) return;
            regia->disconnesso(it->second);
            conn_di.erase(it->second);
            id_di.erase(it);
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& dati, bool binario) {
            if (binario) return;
            std::lock_guard lock(mutex);
            const auto it = id_di.find(&conn);
            if (it == id_di.end()) return;
            try {
                consegna(regia->messaggio(it->second, dati, ora_ms()));
            } catch (const std::exception& e) {
                std::cerr << "errore nel gestire un messaggio: " << e.what() << "\n";
            }
        });

    CROW_ROUTE(app, "/")([] {
        crow::response r;
        r.redirect("/plancia");
        return r;
    });
    CROW_ROUTE(app, "/<path>")([&](const std::string& percorso) { return file_web(opzioni->web, percorso); });

    std::cout << "  plancia:  http://localhost:" << opzioni->porta << "/plancia\n"
              << "  pubblico: http://localhost:" << opzioni->porta << "/pubblico\n"
              << "  debug:    http://localhost:" << opzioni->porta << "/debug\n"
              << std::flush;

    app.bindaddr(opzioni->indirizzo).port(opzioni->porta).multithreaded().run();
    return 0;
}
