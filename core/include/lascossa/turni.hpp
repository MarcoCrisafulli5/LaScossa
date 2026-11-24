// Turno dei concorrenti: chi deve rispondere ora. Agnostico al numero di concorrenti.
#pragma once

namespace lascossa {

class Turni {
public:
    explicit Turni(int numero_concorrenti) : n_(numero_concorrenti > 0 ? numero_concorrenti : 1) {}

    int corrente() const { return corrente_; }
    int numero() const { return n_; }
    void passa() { corrente_ = (corrente_ + 1) % n_; }

private:
    int n_;
    int corrente_ = 0;
};

}  // namespace lascossa
