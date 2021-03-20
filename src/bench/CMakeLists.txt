project(bench_shmem)

set(SOURCE_FILES
        conf.cpp
        libcuckoo.cpp
        main.cpp
        redis.cpp
        shmap.cpp
)

find_package(benchmark REQUIRED)
find_package(hiredis)

add_executable(bench_shmem ${SOURCE_FILES})

set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG -stdlib=libc++")

target_link_libraries(bench_shmem benchmark hiredis pthread rt)
