project(test)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        test.cpp
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(test ${SOURCE_FILES})

if(SHMAPS_SEG_SIZE)
    target_compile_definitions(test PRIVATE "SHMAPS_SEG_SIZE=${SHMAPS_SEG_SIZE}")
endif()

target_link_libraries(test pthread rt)
