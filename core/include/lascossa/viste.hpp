// Proiezioni dello stato: ciò che ogni schermo può vedere.
//
// Il server calcola due viste dalla stessa Partita:
// - VistaPlancia: tutto, comprese soluzioni, eventi ammessi e dati di debug.
// - VistaPubblico: solo ciò che la sala può vedere.
//
// La separazione è garantita dai tipi: le struct del pubblico non hanno campi per la
// soluzione, quindi una soluzione non può arrivare al browser del pubblico per errore.
// La soluzione compare nel pubblico solo come stato di un'opzione già giudicata o rivelata.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lascossa/ciclo_domanda.hpp"
#include "lascossa/esito.hpp"
#include "lascossa/eventi.hpp"
#include "lascossa/partita.hpp"

namespace lascossa {

// Stato grafico di un'opzione (regola giallo -> verde/rosso, RC-06).
enum class StatoOpzione {
    Neutra,
    Selezionata,  // giallo: scelta del concorrente, non ancora confermata
    Giusta,       // verde: confermata e corretta
    Sbagliata,    // rosso: confermata e sbagliata (resta rossa nelle modalità con riprova)
    Soluzione,    // rivelata dal conduttore senza risposta giusta
};

std::string_view nome(StatoOpzione s);

// Timer di gioco per i browser, che disegnano il conto alla rovescia da soli con l'ora del server:
// se corre, tempo che manca = scadenza_ms - ora; altrimenti residuo_ms. Il conto alla rovescia
// visibile è al massimo durata_ms; la parte iniziale (totale_ms - durata_ms) è il ritardo prima
// che parta (in ESA: gli indizi). Uguale per plancia e pubblico.
struct TimerVista {
    std::string stato;  // "spento", "corre", "pausa", "fermo", "scaduto"
    std::int64_t durata_ms = 0;
    std::int64_t totale_ms = 0;
    std::int64_t scadenza_ms = 0;  // ora del server; valida se stato == "corre"
    std::int64_t residuo_ms = 0;   // tempo che mancava alla scadenza, se non corre
};

// ---------------------------------------------------------------- pubblico

struct OpzionePubblico {
    std::string etichetta;
    StatoOpzione stato = StatoOpzione::Neutra;
};

struct EstasPubblico {
    std::string stato;      // stato della macchina Estas: decide quale schermata disegnare
    std::string categoria;  // vuota finché non si sceglie una categoria
    std::string oggetto;    // vuoto se nessun oggetto è in gioco
    std::vector<OpzionePubblico> opzioni;
    int attesa_esito_ms = 0;  // da config: quanto resta gialla un'opzione appena giudicata
};

// ESA: gli indizi arrivano tutti, li svela il browser uno ogni lettura_indizio_ms seguendo il timer.
// La frase c'è solo quando è stata indovinata, rivelata o è scaduto il tempo.
struct EsaPubblico {
    std::string stato;  // stato della macchina ESA
    std::vector<std::string> indizi;  // vuoto in AttesaFrase e Finita
    std::string frase;                // vuota finché la soluzione non può essere mostrata
    TimerVista timer;
    std::int64_t lettura_indizio_ms = 0;
};

struct VistaPubblico {
    bool blackout = false;
    bool riquadro_visibile = false;
    std::string riquadro_testo;
    std::optional<IdModalita> modalita;  // nullopt: schermata di attesa con il logo
    std::optional<EstasPubblico> estas;
    std::optional<EsaPubblico> esa;
};

// ---------------------------------------------------------------- plancia

struct EventoAmmesso {
    TipoEvento tipo;
    int arg = -1;
};

struct ModalitaPlancia {
    IdModalita id;
    bool disponibile = false;  // macchina a stati implementata
    bool giocata = false;      // già giocata in questa serata
};

struct CategoriaPlancia {
    std::string nome;
    std::size_t totale = 0;
    std::size_t giocati = 0;
};

struct OpzionePlancia {
    std::string etichetta;
    StatoOpzione stato = StatoOpzione::Neutra;
    bool corretta = false;  // la plancia vede sempre la soluzione
};

struct EstasPlancia {
    std::string stato;  // stato della macchina Estas
    std::vector<CategoriaPlancia> categorie;
    int categoria_corrente = -1;
    std::optional<std::string> oggetto;
    std::optional<bool> oggetto_appartiene;
    // In AttesaOggetto: l'oggetto che "Mostra oggetto" porterà in gioco, solo per i conduttori.
    std::optional<std::string> anteprima;
    std::optional<bool> anteprima_appartiene;
    std::string stato_domanda;  // vuoto se nessun oggetto è in gioco
    bool domanda_conclusa = false;
    std::vector<OpzionePlancia> opzioni;
    int concorrente_di_turno = 0;  // 0-based; anonimo: la plancia mostra solo il numero
    int numero_concorrenti = 0;
    int attesa_esito_ms = 0;  // da config: la plancia blocca "prossimo" finché il pubblico non colora
};

struct FrasePlancia {
    std::vector<std::string> indizi;
    std::string soluzione;
};

struct EsaPlancia {
    std::string stato;  // stato della macchina ESA
    std::size_t giocate = 0;
    std::size_t totale = 0;
    std::optional<FrasePlancia> frase;      // in gioco o appena conclusa
    std::optional<FrasePlancia> anteprima;  // in AttesaFrase: la prossima, solo per i conduttori
    TimerVista timer;
    std::int64_t lettura_indizio_ms = 0;
};

// Riepilogo dei contenuti caricati, mostrato in plancia prima di iniziare.
struct RiepilogoContenuti {
    std::size_t estas_categorie = 0;
    std::size_t estas_oggetti = 0;
    std::size_t esa_frasi = 0;
};

struct EventoRegistrato {
    std::size_t seq = 0;  // posizione nel registro, da 1
    Evento evento;
};

struct VistaPlancia {
    std::string stato_serata;
    bool pausa = false;
    bool blackout = false;
    bool riquadro_visibile = false;
    std::string riquadro_testo;
    int numero_concorrenti = 0;
    RiepilogoContenuti contenuti;
    std::vector<ModalitaPlancia> modalita;
    std::optional<IdModalita> modalita_attiva;
    std::string stato_modalita;
    std::optional<EstasPlancia> estas;
    std::optional<EsaPlancia> esa;
    std::vector<EventoAmmesso> ammessi;

    // debug
    std::size_t lunghezza_registro = 0;
    std::vector<EventoRegistrato> ultimi_eventi;     // più recenti in fondo
    std::vector<EventoRifiutato> ultimi_rifiutati;   // più recenti in fondo
    std::vector<Transizione> ultime_transizioni;     // più recenti in fondo
};

// Eventi che la serata accetterebbe adesso, con gli argomenti possibili.
// Calcolati provando ogni evento su una copia della serata: la plancia abilita
// solo questi pulsanti, senza avere logica di gioco nel browser.
std::vector<EventoAmmesso> eventi_ammessi(const Partita& p);

VistaPlancia vista_plancia(const Partita& p, std::size_t quanti_debug = 20);
VistaPubblico vista_pubblico(const Partita& p);

}  // namespace lascossa
