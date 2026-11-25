// Cartella dati temporanea per i test: creata vuota, cancellata alla fine.
#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>

namespace lascossa::test {

class CartellaTemporanea {
public:
    CartellaTemporanea() {
        std::random_device rd;
        percorso_ = std::filesystem::temp_directory_path() / ("lascossa_test_" + std::to_string(rd()) + std::to_string(rd()));
        std::filesystem::create_directories(percorso_);
    }
    ~CartellaTemporanea() {
        std::error_code ec;
        std::filesystem::remove_all(percorso_, ec);
    }
    CartellaTemporanea(const CartellaTemporanea&) = delete;
    CartellaTemporanea& operator=(const CartellaTemporanea&) = delete;

    const std::filesystem::path& percorso() const { return percorso_; }

    void scrivi(const std::filesystem::path& relativo, std::string_view contenuto) const {
        const auto p = percorso_ / relativo;
        std::filesystem::create_directories(p.parent_path());
        std::ofstream(p, std::ios::binary) << contenuto;
    }

private:
    std::filesystem::path percorso_;
};

}  // namespace lascossa::test
