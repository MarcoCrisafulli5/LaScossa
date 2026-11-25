#include "registro_file.hpp"

#include <algorithm>
#include <ctime>
#include <sstream>

#include <nlohmann/json.hpp>

#include "csv.hpp"

namespace lascossa::server {

namespace fs = std::filesystem;

namespace {

std::string nome_file(std::chrono::system_clock::time_point ora) {
    const std::time_t t = std::chrono::system_clock::to_time_t(ora);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d_%H-%M-%S", &tm);
    return buf;
}

}  // namespace

RegistroSuFile::RegistroSuFile(fs::path file, std::size_t righe)
    : file_(std::move(file)), out_(file_, std::ios::binary | std::ios::app), righe_(righe) {
    if (!out_) {
        throw ErroreDati("impossibile scrivere il registro " + file_.generic_string());
    }
}

RegistroSuFile RegistroSuFile::nuovo(const fs::path& cartella, std::chrono::system_clock::time_point ora) {
    fs::create_directories(cartella);
    const std::string base = nome_file(ora);
    fs::path p = cartella / (base + ".jsonl");
    for (int n = 2; fs::exists(p); ++n) {
        p = cartella / (base + "_" + std::to_string(n) + ".jsonl");
    }
    return RegistroSuFile(p, 0);
}

RegistroSuFile RegistroSuFile::riprendi(const fs::path& file, std::size_t righe_esistenti) {
    return RegistroSuFile(file, righe_esistenti);
}

std::optional<fs::path> RegistroSuFile::ultimo(const fs::path& cartella) {
    if (!fs::is_directory(cartella)) return std::nullopt;
    std::optional<fs::path> migliore;
    for (const auto& voce : fs::directory_iterator(cartella)) {
        if (!voce.is_regular_file() || voce.path().extension() != ".jsonl") continue;
        if (!migliore || voce.path().filename() > migliore->filename()) migliore = voce.path();
    }
    return migliore;
}

LetturaRegistro RegistroSuFile::leggi(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw ErroreDati("impossibile leggere il registro " + file.generic_string());

    std::vector<std::string> righe;
    for (std::string riga; std::getline(in, riga);) {
        if (!riga.empty() && riga.back() == '\r') riga.pop_back();
        if (!riga.empty()) righe.push_back(std::move(riga));
    }

    LetturaRegistro r;
    for (std::size_t i = 0; i < righe.size(); ++i) {
        const std::string dove = file.filename().generic_string() + " riga " + std::to_string(i + 1);
        try {
            const auto j = nlohmann::json::parse(righe[i]);
            const auto tipo = tipo_da_nome(j.at("evento").get<std::string>());
            if (!tipo) throw ErroreDati(dove + ": evento sconosciuto");
            Evento e{*tipo, j.value("arg", -1), j.value("t", std::int64_t{0}), j.value("testo", std::string())};
            r.eventi.push_back(std::move(e));
        } catch (const nlohmann::json::exception& ex) {
            if (i + 1 == righe.size()) {
                r.avviso = dove + ": riga incompleta ignorata (probabile interruzione durante la scrittura)";
            } else {
                throw ErroreDati(dove + ": riga non valida (" + ex.what() + ")");
            }
        }
    }
    return r;
}

void RegistroSuFile::aggiungi(const Evento& e, std::string_view stato_partenza) {
    ++righe_;
    const nlohmann::json j = {
        {"seq", righe_},
        {"t", e.t_ms},
        {"evento", std::string(nome(e.tipo))},
        {"arg", e.arg},
        {"testo", e.testo},
        {"da", std::string(stato_partenza)},
    };
    out_ << j.dump() << '\n';
    out_.flush();
    if (!out_) {
        throw ErroreDati("scrittura del registro fallita: " + file_.generic_string());
    }
}

}  // namespace lascossa::server
