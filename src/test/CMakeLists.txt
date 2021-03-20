project(run_tests)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        test.cpp
)

set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG -stdlib=libc++")

add_executable(run_tests ${SOURCE_FILES})

target_link_libraries(run_tests pthread rt)
