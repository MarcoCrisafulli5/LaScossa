#include "dati.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>

#include <nlohmann/json.hpp>

namespace lascossa::server {

namespace {

constexpr std::size_t kMinimoOggettiPerCategoria = 11;  // regole: "più di 10 oggetti per categoria"
constexpr int kMassimoConcorrenti = 20;
constexpr int kMassimaAttesaEsitoMs = 10000;  // oltre 10 s la suspense blocca la plancia troppo a lungo

std::string minuscolo(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string posizione(std::string_view file, std::size_t riga) {
    return std::string(file) + " riga " + std::to_string(riga) + ": ";
}

}  // namespace

std::string leggi_file(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        throw ErroreDati("impossibile aprire " + p.generic_string());
    }
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

void leggi_config(std::string_view testo, Configurazione& cfg, Impostazioni& imp) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(testo);
    } catch (const nlohmann::json::parse_error& e) {
        throw ErroreDati(std::string("config.json: JSON non valido (") + e.what() + ")");
    }
    if (!j.is_object()) throw ErroreDati("config.json: deve essere un oggetto JSON");

    if (j.contains("concorrenti")) {
        const auto& c = j["concorrenti"];
        if (!c.is_object() || !c.contains("numero") || !c["numero"].is_number_integer()) {
            throw ErroreDati("config.json: concorrenti.numero deve essere un numero intero");
        }
        const int n = c["numero"].get<int>();
        if (n < 1 || n > kMassimoConcorrenti) {
            throw ErroreDati("config.json: concorrenti.numero deve essere tra 1 e " + std::to_string(kMassimoConcorrenti));
        }
        cfg.numero_concorrenti = n;
    }
    if (j.contains("estas") && j["estas"].contains("attesaEsitoMs")) {
        const auto& a = j["estas"]["attesaEsitoMs"];
        if (!a.is_number_integer() || a.get<int>() < 0 || a.get<int>() > kMassimaAttesaEsitoMs) {
            throw ErroreDati("config.json: estas.attesaEsitoMs deve essere un intero tra 0 e " +
                             std::to_string(kMassimaAttesaEsitoMs));
        }
        cfg.estas.attesa_esito_ms = a.get<int>();
    }
    if (j.contains("plancia") && j["plancia"].contains("pin")) {
        const auto& pin = j["plancia"]["pin"];
        if (pin.is_null()) {
            imp.pin.reset();
        } else if (pin.is_string() && !pin.get<std::string>().empty()) {
            imp.pin = pin.get<std::string>();
        } else {
            throw ErroreDati("config.json: plancia.pin deve essere null o una stringa non vuota");
        }
    }
}

std::vector<CategoriaEstas> leggi_estas(const TabellaCsv& t, std::string_view nome_file,
                                        std::vector<std::string>& avvisi) {
    const std::size_t c_cat = t.colonna("categoria", nome_file);
    const std::size_t c_ogg = t.colonna("oggetto", nome_file);
    const std::size_t c_app = t.colonna("appartiene", nome_file);

    std::vector<CategoriaEstas> categorie;
    std::map<std::string, std::size_t> indice;  // nome categoria -> posizione
    for (const RigaCsv& r : t.righe) {
        const auto campo = [&](std::size_t c) { return c < r.campi.size() ? r.campi[c] : std::string(); };
        const std::string cat = campo(c_cat);
        const std::string ogg = campo(c_ogg);
        const std::string app = minuscolo(campo(c_app));
        if (cat.empty()) throw ErroreDati(posizione(nome_file, r.numero) + "categoria vuota");
        if (ogg.empty()) throw ErroreDati(posizione(nome_file, r.numero) + "oggetto vuoto");

        bool appartiene = false;
        if (app == "si" || app == "s\xC3\xAC" || app == "s\xC3\x8C") {  // si, sì, sÌ
            appartiene = true;
        } else if (app == "no") {
            appartiene = false;
        } else {
            throw ErroreDati(posizione(nome_file, r.numero) + "appartiene deve essere si o no, trovato \"" +
                             campo(c_app) + "\"");
        }

        auto [it, nuova] = indice.try_emplace(cat, categorie.size());
        if (nuova) categorie.push_back({cat, {}});
        auto& oggetti = categorie[it->second].oggetti;
        if (std::any_of(oggetti.begin(), oggetti.end(), [&](const OggettoEstas& o) { return o.nome == ogg; })) {
            avvisi.push_back(posizione(nome_file, r.numero) + "\"" + ogg + "\" ripetuto nella categoria \"" + cat + "\"");
        }
        oggetti.push_back({ogg, appartiene});
    }
    for (const CategoriaEstas& c : categorie) {
        if (c.oggetti.size() < kMinimoOggettiPerCategoria) {
            avvisi.push_back(std::string(nome_file) + ": la categoria \"" + c.nome + "\" ha " +
                             std::to_string(c.oggetti.size()) + " oggetti (le regole ne chiedono più di 10)");
        }
    }
    return categorie;
}

DatiSerata carica_dati(const std::filesystem::path& cartella) {
    DatiSerata d;
    const auto config = cartella / "config.json";
    if (!std::filesystem::exists(config)) {
        throw ErroreDati("manca " + config.generic_string());
    }
    leggi_config(leggi_file(config), d.configurazione, d.impostazioni);

    const auto estas = cartella / "contenuti" / "mod1_estas.csv";
    if (!std::filesystem::exists(estas)) {
        throw ErroreDati("manca " + estas.generic_string());
    }
    d.contenuti.estas = leggi_estas(leggi_csv(leggi_file(estas), "mod1_estas.csv"), "mod1_estas.csv", d.avvisi);
    if (d.contenuti.estas.empty()) {
        throw ErroreDati("mod1_estas.csv: nessuna categoria");
    }
    return d;
}

}  // namespace lascossa::server
