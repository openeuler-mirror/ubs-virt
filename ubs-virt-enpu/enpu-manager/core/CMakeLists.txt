cmake_minimum_required(VERSION 3.15)
project(enpu-manager LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH TRUE)

message(STATUS "Building enpu-manager")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.c)
list(APPEND SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/src/allocation_id.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/src/enpu_dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/src/vnpu_metrics_reader.c)
file(GLOB_RECURSE HEADER_FILES include/*.h)

set(COMPILE_OPTIONS
    -Wall
    -O2
    -fPIC
    -fstack-protector-strong
)

include(${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/securec.cmake)

set(LINK_LIBS pthread rt dl cjson c_sec)

add_library(enpu_manager SHARED ${SRC_FILES})
set_target_properties(enpu_manager PROPERTIES
    OUTPUT_NAME enpu_manager
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(enpu_manager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(enpu_manager PRIVATE ${LINK_LIBS})
target_compile_options(enpu_manager PRIVATE ${COMPILE_OPTIONS})

option(BUILD_TESTS "Build unit tests" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(BUILD_TESTS)
    enable_testing()
    if(ENABLE_COVERAGE)
        message(STATUS "Coverage enabled")
        target_compile_options(enpu_manager PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_options(enpu_manager PRIVATE --coverage)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
        add_subdirectory(test)
    else()
        file(GLOB TEST_FILES test/*.c test/*.cpp)
        if(TEST_FILES)
            add_executable(enpu_manager_test ${TEST_FILES} ${SRC_FILES})
            target_include_directories(enpu_manager_test PRIVATE 
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/internal
                ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include)
            target_link_libraries(enpu_manager_test PRIVATE ${LINK_LIBS})
            if(ENABLE_COVERAGE)
                target_compile_options(enpu_manager_test PRIVATE --coverage -fprofile-arcs -ftest-coverage)
                target_link_options(enpu_manager_test PRIVATE --coverage)
            endif()
            add_test(NAME enpu_manager_test COMMAND enpu_manager_test)
        endif()
    endif()
endif()

message(STATUS "CMakeLists.txt configuration complete")
