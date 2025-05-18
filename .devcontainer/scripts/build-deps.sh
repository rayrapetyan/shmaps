#!/usr/bin/env bash

set -eux

BUILD_TYPE=${BUILD_TYPE:-Debug}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
NUM_BUILD_WORKERS=${NUM_BUILD_WORKERS:-$(nproc)}
SRC_DIR="/src"

apt-get install -y --no-install-recommends \
        autoconf \
        build-essential \
        cmake \
        libc++-dev \
        libc++abi-dev \
        libcap2-bin \
        libtool \
        git \
        wget

# all cpp deps must be recompiled with -stdlib=libc++ (libcuckoo's requirement)
# so, once shmaps is working with libstdc++, we can drop builds below and install them via apt
export CXXFLAGS="-I$INSTALL_PREFIX/include -std=c++11 -stdlib=libc++"
export LDFLAGS="-L$INSTALL_PREFIX/lib"

update-alternatives --install /usr/bin/cc cc /usr/bin/clang 60
update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 60

# boost deps
cd "$SRC_DIR"
wget https://archives.boost.io/release/1.80.0/source/boost_1_80_0.tar.gz \
    && tar -xzf boost_1_80_0.tar.gz \
    && cd boost_1_80_0 \
    && ./bootstrap.sh --with-toolset=clang --prefix=$INSTALL_PREFIX \
    && ./b2 headers \
    && ./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" --with-filesystem --with-thread --with-atomic install

cd "$SRC_DIR"
git clone --branch erase_random https://github.com/rayrapetyan/libcuckoo.git \
    && cd libcuckoo \
    && cp -r libcuckoo $INSTALL_PREFIX/include
