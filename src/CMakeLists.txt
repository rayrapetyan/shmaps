cmake_minimum_required(VERSION 3.12)

project(shmaps)

set(CMAKE_VERBOSE_MAKEFILE "ON")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "-O0 -g -fsanitize=address")
else()
    set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

enable_testing()

add_subdirectory(bench)
add_subdirectory(test)
