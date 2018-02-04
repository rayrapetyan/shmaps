cmake_minimum_required(VERSION 3.3)
project(bench_shmem)

set(CMAKE_VERBOSE_MAKEFILE "ON")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall")

set(SOURCE_FILES
        main.cpp
        redis.cpp
        #libcuckoo.cpp
        shmap.cpp
        conf.cpp
        ../shmaps.cpp)

include_directories(/usr/local/include)
include_directories(../)
#include_directories(/ara/devel/3rd_party/libcuckoo)

link_directories(/usr/local/lib)

add_executable(bench_shmem ${SOURCE_FILES})

target_link_libraries(bench_shmem benchmark hiredis)
