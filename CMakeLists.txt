cmake_minimum_required(VERSION 3.7)

project(shmaps)

set(CMAKE_VERBOSE_MAKEFILE "ON")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "-O0 -g")
else()
    set(CMAKE_CXX_FLAGS "-O2 -DNDEBUG")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")

include_directories(/usr/local/include)
include_directories(/ara/devel/shmaps/include)
include_directories(/ara/devel/3rd_party/libcuckoo)

add_subdirectory(src/bench)
add_subdirectory(src/test)

set(SOURCE_FILES
        src/shmaps.cpp
)

add_library(shmaps STATIC ${SOURCE_FILES})
