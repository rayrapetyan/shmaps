# https://github.com/devcontainers/images/tree/main/src/cpp
ARG VARIANT=debian-12
FROM mcr.microsoft.com/devcontainers/cpp:${VARIANT}

WORKDIR /devcontainer

RUN apt update && apt install -y --no-install-recommends \
        libbenchmark-dev \
        libhiredis-dev

WORKDIR /src

COPY scripts/build-deps.sh .
RUN ./build-deps.sh

RUN rm -rf /var/lib/apt/lists/*

WORKDIR /worksapces/shmaps
