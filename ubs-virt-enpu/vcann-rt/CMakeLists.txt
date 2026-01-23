cmake_minimum_required(VERSION 3.15)
project(vnpu)

message([DEBUG] "CMakeLists.txt start ${CMAKE_CURRENT_SOURCE_DIR}")

# add_subdirectory(src)

option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

message([DEBUG] "CMakeLists.txt end ${CMAKE_CURRENT_SOURCE_DIR}")
