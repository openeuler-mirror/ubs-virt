cmake_minimum_required(VERSION 3.15)
project(vcann-runtime)

message(STATUS "CMakeList start ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SRC_LIST
    src/*.c
    src/ascend/*.c
    src/tools/*.c
    src/utils/*.c
)

add_compile_options(
    -O2
    -Wall
    -Wextra
    -Wformat-nonliteral
    -Wformat-security
    -Wformat-y2k
    -Wfloat-equal
    -std=gnu11
    -fPIC
    -fstack-protector-strong
    -fvisibility=hidden
    -fno-common
)

include(${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/securec.cmake)

add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)

set(INCLUDE_DIRS
    $ENV{ASCEND_TOOLKIT_HOME}/include
    $ENV{ASCEND_HOME_PATH}/../../driver/include
    $ENV{ASCEND_HOME_PATH}/include/experiment/runtime
    $ENV{ASCEND_HOME_PATH}/include/experiment/msprof
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

set(LINK_DIRS
    $ENV{ASCEND_HOME_PATH}/runtime
    $ENV{ASCEND_HOME_PATH}/lib64
    $ENV{ASCEND_HOME_PATH}/../../driver/lib64/driver
)

set(LINK_LIBS dcmi)

message(STATUS "CMakeList build start")
add_library(vruntime SHARED src/ascend/hook.c ${SRC_LIST})

target_link_options(vruntime PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

target_include_directories(vruntime PRIVATE ${INCLUDE_DIRS})
target_link_directories(vruntime PRIVATE ${LINK_DIRS})
target_link_libraries(vruntime PRIVATE securec ${LINK_LIBS})

add_executable(enpu-monitor src/tools/monitor.c ${SRC_LIST})

target_link_options(enpu-monitor PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

target_include_directories(enpu-monitor PRIVATE ${INCLUDE_DIRS})
target_link_directories(enpu-monitor PRIVATE ${LINK_DIRS})
target_link_libraries(enpu-monitor PRIVATE securec ${LINK_LIBS})

option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

message(STATUS "CMakeList end")