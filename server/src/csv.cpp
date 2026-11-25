#include "csv.hpp"

#include <algorithm>
#include <cctype>

namespace lascossa::server {

namespace {

std::string pulisci(std::string s) {
    const auto spazio = [](unsigned char c) { return std::isspace(c) != 0; };
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), spazio));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), spazio).base(), s.end());
    return s;
}

std::string minuscolo(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool vuota(const std::vector<std::string>& campi) {
    return std::all_of(campi.begin(), campi.end(), [](const std::string& c) { return c.empty(); });
}

}  // namespace

bool utf8_valido(std::string_view t) {
    std::size_t i = 0;
    while (i < t.size()) {
        const auto c = static_cast<unsigned char>(t[i]);
        std::size_t n = 0;
        if (c < 0x80) n = 0;
        else if ((c & 0xE0) == 0xC0 && c >= 0xC2) n = 1;
        else if ((c & 0xF0) == 0xE0) n = 2;
        else if ((c & 0xF8) == 0xF0 && c <= 0xF4) n = 3;
        else return false;
        if (i + n >= t.size() && n > 0) return false;
        for (std::size_t k = 1; k <= n; ++k) {
            if ((static_cast<unsigned char>(t[i + k]) & 0xC0) != 0x80) return false;
        }
        i += n + 1;
    }
    return true;
}

std::size_t TabellaCsv::colonna(std::string_view nome, std::string_view nome_file) const {
    const auto it = std::find(intestazione.begin(), intestazione.end(), nome);
    if (it == intestazione.end()) {
        throw ErroreDati(std::string(nome_file) + ": manca la colonna \"" + std::string(nome) + "\" nell'intestazione");
    }
    return static_cast<std::size_t>(it - intestazione.begin());
}

TabellaCsv leggi_csv(std::string_view testo, std::string_view nome_file) {
    if (testo.substr(0, 3) == "\xEF\xBB\xBF") {
        testo.remove_prefix(3);
    }
    if (!utf8_valido(testo)) {
        throw ErroreDati(std::string(nome_file) +
                         ": il file non è in UTF-8. In Excel salvalo come \"CSV UTF-8 (delimitato da virgole)\"");
    }

    std::vector<RigaCsv> righe;
    RigaCsv corrente{1, {}};
    std::string campo;
    bool tra_virgolette = false;
    bool era_tra_virgolette = false;
    std::size_t riga = 1;

    const auto chiudi_campo = [&] {
        corrente.campi.push_back(era_tra_virgolette ? campo : pulisci(campo));
        campo.clear();
        era_tra_virgolette = false;
    };
    const auto chiudi_riga = [&] {
        chiudi_campo();
        if (!vuota(corrente.campi)) righe.push_back(std::move(corrente));
        corrente = RigaCsv{riga + 1, {}};
    };

    for (std::size_t i = 0; i < testo.size(); ++i) {
        const char c = testo[i];
        if (tra_virgolette) {
            if (c == '"') {
                if (i + 1 < testo.size() && testo[i + 1] == '"') {
                    campo += '"';
                    ++i;
                } else {
                    tra_virgolette = false;
                }
            } else {
                if (c == '\n') ++riga;
                campo += c;
            }
            continue;
        }
        switch (c) {
            case '"':
                if (pulisci(campo).empty()) {
                    campo.clear();
                    tra_virgolette = true;
                    era_tra_virgolette = true;
                } else {
                    campo += c;
                }
                break;
            case ';': chiudi_campo(); break;
            case '\r': break;
            case '\n':
                chiudi_riga();
                ++riga;
                break;
            default: campo += c;
        }
    }
    if (tra_virgolette) {
        throw ErroreDati(std::string(nome_file) + " riga " + std::to_string(corrente.numero) +
                         ": virgolette aperte e mai chiuse");
    }
    chiudi_riga();

    if (righe.empty()) {
        throw ErroreDati(std::string(nome_file) + ": file vuoto, manca l'intestazione");
    }
    TabellaCsv t;
    for (std::string& nome : righe.front().campi) t.intestazione.push_back(minuscolo(pulisci(nome)));
    t.righe.assign(std::make_move_iterator(righe.begin() + 1), std::make_move_iterator(righe.end()));
    return t;
}

}  // namespace lascossa::server
