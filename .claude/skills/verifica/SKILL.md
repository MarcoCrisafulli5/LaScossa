---
name: verifica
description: Verifica completa di La Scossa prima di dichiarare finita una modifica o di fare commit - build C++ con -Werror, test doctest di core e server, typecheck e build della web app, prova end-to-end del protocollo. Usala dopo ogni modifica al codice.
---

# Verifica completa

Esegui i passi che riguardano ciò che hai toccato; nel dubbio, tutti. Riporta all'utente i numeri
reali (test passati, asserzioni, eventuali errori), non un generico "funziona".

Metti build e file temporanei nella scratchpad della sessione, mai nel repo.

## Scorciatoia: tutto in Docker

`docker compose build` esegue build web, build C++ con -Werror e `ctest`: se passa, i punti 1 e 2
sono coperti senza ambiente locale. Per l'e2e usa un container usa-e-getta su una copia dei dati,
per non toccare la partita in `dati/partite/`:

```sh
docker run -d --rm --name lascossa-e2e -p 127.0.0.1:8099:8080 -v "<scratchpad>/dati-copia:/dati" lascossa
node server/test/e2e.mjs 8099 && docker stop lascossa-e2e
```

## 1. C++ (sempre, se hai toccato core/, server/ o CMakeLists.txt)

Con CMake e Ninja:

```sh
cmake -S . -B <scratchpad>/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build <scratchpad>/build
ctest --test-dir <scratchpad>/build --output-on-failure
```

Se CMake non è installato (su Windows: `pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`
in MSYS2, oppure `pip install cmake ninja` in un venv nella scratchpad), compila a mano con i flag
in `.claude/rules/cpp.md`. Zero warning: la build usa `-Werror`.

## 2. Web (se hai toccato web/ o il protocollo)

```sh
cd web && npm run build   # tsc --noEmit + vite build
```

## 3. End-to-end (se hai toccato server/, il protocollo o una modalità)

```sh
cp -r dati <scratchpad>/dati-e2e
./lascossa-server --dati <scratchpad>/dati-e2e --web web/dist --porta 8099 --nuova   # in background
node server/test/e2e.mjs 8099
```

Poi ferma il server. Se hai aggiunto una modalità, estendi `server/test/e2e.mjs` con il suo giro
completo e con il controllo che il pubblico non riceva la soluzione.

## 4. Coerenza

- Protocollo cambiato? `viste.hpp`, `json_viste.cpp` e `protocollo.ts` allineati.
- Nuovi `.cpp` in `CMakeLists.txt`.
- Documentazione: `core/README.md`, "Stato" in `CLAUDE.md`, `README.md` se è cambiato cosa funziona.
- `CHANGELOG.md`: la modifica ha la sua riga in "Non rilasciato". Se è un rilascio, `VERSION` e il
  tag in `docker-compose.yml` coincidono.
