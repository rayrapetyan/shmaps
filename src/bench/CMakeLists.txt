project(bench)

cmake_minimum_required(VERSION 3.18.4)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        conf.cpp
        libcuckoo.cpp
        main.cpp
        redis.cpp
        shmap.cpp
        )

find_package(benchmark REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(bench ${SOURCE_FILES})

if(SHMAPS_SEG_SIZE)
    target_compile_definitions(bench PRIVATE "SHMAPS_SEG_SIZE=${SHMAPS_SEG_SIZE}")
endif()

target_link_libraries(bench benchmark hiredis pthread rt)
