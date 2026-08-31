FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libssl-dev \
    nlohmann-json3-dev \
    libminiaudio-dev \
    libpulse-dev

WORKDIR /

COPY . .

RUN cmake -S . -B build && \
    cmake --build build



FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libpulse0 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /

COPY --from=build /build/src/bell /usr/local/bin/bell

EXPOSE 80

CMD ["/usr/local/bin/bell"]
