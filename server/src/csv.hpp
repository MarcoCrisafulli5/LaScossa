// Lettore CSV minimo per i contenuti: separatore punto e virgola, UTF-8, una riga per record.
// È il formato che Excel in italiano apre e salva ("CSV UTF-8"). Nessuna libreria esterna.
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lascossa::server {

// Errore nei file di dati: il messaggio dice file, riga e problema, ed è pensato per chi
// prepara i contenuti (es. "mod1_estas.csv riga 12: appartiene deve essere si o no").
class ErroreDati : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct RigaCsv {
    std::size_t numero = 0;  // riga del file, da 1, per i messaggi di errore
    std::vector<std::string> campi;
};

struct TabellaCsv {
    std::vector<std::string> intestazione;  // nomi di colonna in minuscolo, senza spazi ai lati
    std::vector<RigaCsv> righe;             // righe vuote escluse

    // Indice della colonna con quel nome; lancia ErroreDati se manca.
    std::size_t colonna(std::string_view nome, std::string_view nome_file) const;
};

// Interpreta il testo di un file CSV. Gestisce BOM UTF-8, fine riga \r\n e \n, campi tra
// virgolette ("" per una virgoletta). Lancia ErroreDati se il testo non è UTF-8 valido o
// se manca l'intestazione.
TabellaCsv leggi_csv(std::string_view testo, std::string_view nome_file);

bool utf8_valido(std::string_view testo);

}  // namespace lascossa::server
