# syntax=docker/dockerfile:1

FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN rm -rf build \
    && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel

FROM ubuntu:24.04 AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends netcat-openbsd \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --uid 10001 tcpft \
    && mkdir -p /data \
    && chown -R tcpft:tcpft /data

COPY --from=builder /src/build/ft-server /usr/local/bin/ft-server
COPY --from=builder /src/build/ft-client /usr/local/bin/ft-client

USER tcpft
WORKDIR /data

EXPOSE 9000/tcp

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD nc -z 127.0.0.1 9000 || exit 1

ENTRYPOINT ["ft-server"]
CMD ["9000", "/data"]
