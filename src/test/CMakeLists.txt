set(SOURCE_FILES        
        test.cpp
)

if (CMAKE_BUILD_TYPE MATCHES Release)
    link_directories(/ara/devel/shmaps/cmake-build-release)
elseif (CMAKE_BUILD_TYPE MATCHES Debug)
    link_directories(/ara/devel/shmaps/cmake-build-debug)
endif ()

add_executable(run_tests ${SOURCE_FILES})

target_link_libraries(run_tests shmaps)
