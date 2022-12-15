# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.195.0/containers/cpp/.devcontainer/base.Dockerfile
# [Choice] Debian / Ubuntu version (use Debian 11/9, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, debian-9, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT=debian-11
FROM mcr.microsoft.com/devcontainers/cpp:0-${VARIANT}

WORKDIR /devcontainer

COPY base-scripts/build-deps.sh .
COPY base-scripts/deps.apt .

RUN ./build-deps.sh Debug /usr

RUN apt update \
    && apt install -y \
        clang-format \
        libbenchmark-dev \
    && rm -rf /var/lib/apt/lists/*
