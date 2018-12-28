project(bench_shmem)

set(SOURCE_FILES
        conf.cpp
        libcuckoo.cpp
        main.cpp
        redis.cpp
        shmap.cpp
)

add_executable(bench_shmem ${SOURCE_FILES})
target_link_libraries(bench_shmem benchmark hiredis)
