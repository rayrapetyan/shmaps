#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_TYPE=$1
INSTALL_PREFIX=$2

# all cpp deps must be recompiled with -stdlib=libc++ (libcuckoo's requirement)
# so, once shmaps is working with libstdc++, we can drop most of the builds below and install them via apt
export CXXFLAGS="-I$INSTALL_PREFIX/include -std=c++11 -stdlib=libc++"
export LDFLAGS="-L$INSTALL_PREFIX/lib"

apt update \
    && apt install -y $(awk '{print $1}' deps.apt) \
    && rm -rf /var/lib/apt/lists/*

update-alternatives --install /usr/bin/cc cc /usr/bin/clang-11 60
update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-11 60

# boost deps
ln -s /usr/bin/clang++-11 /usr/bin/clang++
wget https://archives.boost.io/release/1.80.0/source/boost_1_80_0.tar.gz \
    && tar -xzf boost_1_80_0.tar.gz \
    && cd boost_1_80_0 \
    && ./bootstrap.sh --with-toolset=clang --prefix=$INSTALL_PREFIX \
    && ./b2 headers \
    && ./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" --with-filesystem --with-thread --with-atomic install
cd ..

git clone --branch erase_random https://github.com/rayrapetyan/libcuckoo.git \
    && cd libcuckoo \
    && cp -r libcuckoo $INSTALL_PREFIX/include
cd ..
