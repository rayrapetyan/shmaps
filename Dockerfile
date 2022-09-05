# syntax=docker/dockerfile:1
FROM debian:bullseye as builder

ARG APP_HOME=/opt/adp/shmaps

ARG BOOST_VER=1_80_0

WORKDIR $APP_HOME/deps
COPY requirements.apt .

RUN apt update \
    && apt install -y $(awk '{print $1}' requirements.apt) \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-11 60
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-11 60

# boost deps
RUN ln -s /usr/bin/clang++-11 /usr/bin/clang++
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_$BOOST_VER.tar.gz \
    && tar -xzf boost_$BOOST_VER.tar.gz \
    && cd boost_$BOOST_VER \
    && ./bootstrap.sh --with-toolset=clang --prefix=/usr \
    && ./b2 headers \
    && ./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" --with-filesystem --with-thread --with-atomic install

RUN git clone --branch erase_random https://github.com/rayrapetyan/libcuckoo.git libcuckoo \
    && mkdir /usr/local/include/libcuckoo \
    && cp libcuckoo/libcuckoo/*.hh /usr/local/include/libcuckoo

RUN git clone --branch v1.7.0 https://github.com/google/benchmark.git benchmark \
    && cd benchmark \
    && cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_USE_LIBCXX=ON -S . -B "build" \
    && cmake --build "build" --config Release --target install

WORKDIR $APP_HOME

COPY . .

RUN cmake -E make_directory "build" \
    && cmake -DCMAKE_BUILD_TYPE=Release -S . -B "build" \
    && cmake --build "build" --config Release
