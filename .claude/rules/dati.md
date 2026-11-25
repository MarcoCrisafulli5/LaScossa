---
paths:
  - "dati/**"
---

# File di dati

Formato definito nella sezione 4.1 dei requisiti. Chi li prepara non è un programmatore e usa Excel.

- CSV con separatore **punto e virgola**, codifica **UTF-8** ("CSV UTF-8" in Excel), prima riga di
  intestazione con i nomi di colonna; l'ordine delle colonne è libero.
- Un file per modalità in `dati/contenuti/`: `mod1_estas.csv` … `mod8_duelli.csv`, più `concorrenti.csv`.
- Colonne previste:

| File | Colonne |
| --- | --- |
| mod1_estas.csv | categoria; oggetto; appartiene (si/no) |
| mod2_esa.csv | indizio1; indizio2; indizio3; soluzione |
| mod3_date.csv | round; anno1; anno2; anno3; anno4; domanda; anno_corretto |
| mod4_lascossa.csv | round; domanda; alt1 … alt9; sbagliata (1-9) |
| mod5_triello.csv | materia; domanda; a; b; c; d; corretta (a-d); valore (punti o SCOSSA) |
| mod6_centos.csv | domanda; risposta |
| mod7_ghigliottina.csv | round; parola_a; parola_b; corretta (a/b); soluzione_finale |
| mod8_duelli.csv | parola; definizione |
| concorrenti.csv | nome; foto (nome file in dati/foto/) |

- `config.json`: parametri di gioco con valori di default; campi sconosciuti ignorati.
- `dati/partite/` contiene i registri delle serate: non modificarli a mano, non versionarli.
- I contenuti di esempio devono essere corretti nel merito (es. nessun fiume che sia italiano e
  straniero insieme): sbagliare una soluzione in sala è il danno peggiore.
