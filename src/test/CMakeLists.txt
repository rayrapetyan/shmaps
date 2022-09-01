project(test)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        test.cpp
)

set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG -stdlib=libc++ -DSHMAPS_SEG_SIZE=2147483648")

add_executable(test ${SOURCE_FILES})

target_link_libraries(test pthread rt)
