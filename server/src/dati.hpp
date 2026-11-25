// Caricamento e validazione dei dati della serata dalla cartella dati/:
//   config.json               parametri di gioco
//   contenuti/mod1_estas.csv  categoria;oggetto;appartiene
// Un errore blocca l'avvio con un messaggio che indica file e riga.
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "csv.hpp"
#include "lascossa/serata.hpp"

namespace lascossa::server {

// Impostazioni che riguardano il server e non le macchine a stati.
struct Impostazioni {
    std::optional<std::string> pin;  // PIN della plancia; nullopt = nessun PIN (default)
};

struct DatiSerata {
    Configurazione configurazione;
    Contenuti contenuti;
    Impostazioni impostazioni;
    std::vector<std::string> avvisi;  // problemi che non bloccano (es. categoria con pochi oggetti)
};

// Legge config.json. Campi sconosciuti ignorati (servono alle modalità future).
void leggi_config(std::string_view testo, Configurazione& cfg, Impostazioni& imp);

// Colonne: categoria;oggetto;appartiene (si/sì/no). Le categorie seguono l'ordine del file.
std::vector<CategoriaEstas> leggi_estas(const TabellaCsv& t, std::string_view nome_file,
                                        std::vector<std::string>& avvisi);

DatiSerata carica_dati(const std::filesystem::path& cartella);

std::string leggi_file(const std::filesystem::path& p);

}  // namespace lascossa::server
