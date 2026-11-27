# Immagine di La Scossa: un binario C++ e la build statica di React.
#   docker compose up        poi http://localhost:8080/plancia e /pubblico
# Multi-architettura (Mac M4 = linux/arm64, PC = linux/amd64):
#   docker buildx build --platform linux/arm64,linux/amd64 -t lascossa:$(cat VERSION) .

# --- stadio 1: interfaccia React (Node serve solo qui) ---
# Node 24 porta npm 11, lo stesso che genera web/package-lock.json: con npm 10 "npm ci"
# rifiuta il lockfile (dipendenze opzionali risolte in modo diverso).
FROM node:24-alpine AS web
WORKDIR /web
COPY web/package.json web/package-lock.json ./
RUN npm ci
COPY web/ ./
RUN npm run build

# --- stadio 2: server C++ e test ---
FROM debian:trixie-slim AS cpp
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt VERSION ./
COPY third_party third_party
COPY core core
COPY server server
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build \
    && ctest --test-dir build --output-on-failure

# --- immagine finale: niente Node, niente compilatore ---
FROM debian:trixie-slim
# Fuso orario per i nomi dei registri in dati/partite/ (altrimenti UTC).
RUN apt-get update \
    && apt-get install -y --no-install-recommends tzdata \
    && rm -rf /var/lib/apt/lists/*
ENV TZ=Europe/Rome
WORKDIR /app
COPY --from=cpp /src/build/lascossa-server ./lascossa-server
COPY --from=web /web/dist ./web
# Dati di esempio dentro l'immagine; in sala si monta la cartella dati/ vera su /dati.
COPY dati/config.json /dati/config.json
COPY dati/contenuti /dati/contenuti
EXPOSE 8080
CMD ["/app/lascossa-server", "--dati", "/dati", "--web", "/app/web", "--indirizzo", "0.0.0.0", "--porta", "8080"]
