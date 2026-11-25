// Registro degli eventi su disco: la fonte di verità della partita.
// Un file .jsonl per partita in dati/partite/, una riga per evento accettato:
//   {"seq":3,"t":1764000000000,"evento":"SELEZIONA","arg":0,"testo":"","da":"InGioco/OggettoInGioco"}
// Ogni riga è scritta e svuotata su disco prima di aggiornare gli schermi.
// ANNULLA è registrato come ogni altro evento: rigiocando le righe in ordine con
// Partita::applica si ottiene esattamente lo stesso stato, e gli eventi annullati restano nel file.
#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "lascossa/eventi.hpp"

namespace lascossa::server {

struct LetturaRegistro {
    std::vector<Evento> eventi;
    std::optional<std::string> avviso;  // es. ultima riga troncata da un crash, ignorata
};

class RegistroSuFile {
public:
    // Crea un file nuovo partite/AAAA-MM-GG_HH-MM-SS.jsonl.
    static RegistroSuFile nuovo(const std::filesystem::path& cartella, std::chrono::system_clock::time_point ora);
    // Continua a scrivere in fondo a un file esistente.
    static RegistroSuFile riprendi(const std::filesystem::path& file, std::size_t righe_esistenti);
    // Il file di partita più recente (per nome), se esiste.
    static std::optional<std::filesystem::path> ultimo(const std::filesystem::path& cartella);
    // Legge gli eventi di un file. Una riga malformata in mezzo è un errore (ErroreDati);
    // l'ultima riga malformata è ignorata con un avviso (scrittura interrotta da un crash).
    static LetturaRegistro leggi(const std::filesystem::path& file);

    void aggiungi(const Evento& e, std::string_view stato_partenza);

    const std::filesystem::path& file() const { return file_; }
    std::size_t righe() const { return righe_; }

private:
    RegistroSuFile(std::filesystem::path file, std::size_t righe);

    std::filesystem::path file_;
    std::ofstream out_;
    std::size_t righe_ = 0;
};

}  // namespace lascossa::server
