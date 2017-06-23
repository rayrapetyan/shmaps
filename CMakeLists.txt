cmake_minimum_required(VERSION 3.7)
project(shmaps)

set(CMAKE_VERBOSE_MAKEFILE "ON")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -pthread -Wall")

# libcuckoo
set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(/usr/local/include)

add_subdirectory(bench)

set(SOURCE_FILES
        test.cpp
        shmaps.cpp)

add_executable(test_shmaps ${SOURCE_FILES})

#target_link_libraries(test_shmaps)
