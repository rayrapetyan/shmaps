# syntax=docker/dockerfile:1
FROM debian:bookworm-slim as builder

ARG SHMAPS_SEG_SIZE

WORKDIR /src

COPY .devcontainer/scripts/build-deps.sh .

RUN ./build-deps.sh



FROM builder as reset

ARG BUILD_TYPE=Release
ARG SHMAPS_SEG_SIZE

WORKDIR /build

COPY include/ include/
COPY src/reset/ src/reset/

WORKDIR /build/src/reset

RUN cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DSHMAPS_SEG_SIZE=$SHMAPS_SEG_SIZE -S . -B "build" \
    && cmake --build "build" --config $BUILD_TYPE



FROM builder as test

ARG BUILD_TYPE=Release
ARG SHMAPS_SEG_SIZE

ENV CXXFLAGS="-std=c++20 -stdlib=libc++"

WORKDIR /build

COPY include/ include/
COPY src/test/ src/test/

WORKDIR /build/src/test

RUN cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DSHMAPS_SEG_SIZE=$SHMAPS_SEG_SIZE -S . -B "build" \
    && cmake --build "build" --config $BUILD_TYPE



FROM builder as bench

ARG BUILD_TYPE=Release
ARG SHMAPS_SEG_SIZE

ENV CXXFLAGS="-std=c++20 -stdlib=libc++"

WORKDIR /build

RUN apt update \
    && apt install -y \
    libhiredis-dev \
    redis-server \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --branch v1.7.1 https://github.com/google/benchmark.git benchmark \
    && cd benchmark \
    && cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_USE_LIBCXX=ON -S . -B "build" \
    && cmake --build "build" --config $BUILD_TYPE --target install

COPY include/ include/
COPY src/bench/ src/bench/

WORKDIR /build/src/bench

RUN cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DSHMAPS_SEG_SIZE=$SHMAPS_SEG_SIZE -S . -B "build" \
    && cmake --build "build" --config $BUILD_TYPE
