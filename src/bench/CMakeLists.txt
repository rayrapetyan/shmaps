project(bench)

set(CMAKE_CXX_STANDARD 17)

include_directories(/ara/devel/adplatform/adpx/deps/libcuckoo)

set(SOURCE_FILES
        conf.cpp
        libcuckoo.cpp
        main.cpp
        redis.cpp
        shmap.cpp
        )

find_package(benchmark REQUIRED)

add_executable(bench ${SOURCE_FILES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSHMAPS_SEG_SIZE=2147483648")

target_link_libraries(bench benchmark hiredis pthread rt)
