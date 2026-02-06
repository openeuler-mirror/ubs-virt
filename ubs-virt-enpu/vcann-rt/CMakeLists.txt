cmake_minimum_required(VERSION 3.15)
project(vcann-runtime)

message(STATUS "CMakeList start ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SRC_LIST
    src/*.c
    src/ascend/*.c
    src/tools/*.c
    src/utils/*.c
)

set(SECURE_COMPILE_OPTIONS
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

if(ENABLE_NEW_BUILD)
    message(STATUS "CANN version greater than or equal to 8.5.0, using new include dirs.")
    set(INCLUDE_DIRS
        $ENV{ASCEND_TOOLKIT_HOME}/include
        $ENV{ASCEND_HOME_PATH}/../driver/include
        $ENV{ASCEND_HOME_PATH}/pkg_inc/runtime
        $ENV{ASCEND_HOME_PATH}/pkg_inc/profiling
        $ENV{ASCEND_HOME_PATH}/pkg_inc/
        $ENV{ASCEND_HOME_PATH}/pkg_inc/msprof
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    )
    set(LINK_DIRS
        $ENV{ASCEND_HOME_PATH}/runtime
        $ENV{ASCEND_HOME_PATH}/lib64
        $ENV{ASCEND_HOME_PATH}/../driver/lib64/driver
    )
else()
    message(STATUS "CANN version less than 8.5.0, using old include dirs.")
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
endif()



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
target_compile_options(vruntime PRIVATE ${SECURE_COMPILE_OPTIONS})
target_compile_definitions(vruntime PRIVATE _GLIBCXX_USE_CXX11_ABI=0)

add_executable(enpu-monitor src/tools/monitor.c ${SRC_LIST})

target_link_options(enpu-monitor PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

target_include_directories(enpu-monitor PRIVATE ${INCLUDE_DIRS})
target_link_directories(enpu-monitor PRIVATE ${LINK_DIRS})
target_link_libraries(enpu-monitor PRIVATE securec ${LINK_LIBS})
target_compile_options(enpu-monitor PRIVATE ${SECURE_COMPILE_OPTIONS})
target_compile_definitions(enpu-monitor PRIVATE _GLIBCXX_USE_CXX11_ABI=0)

option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

message(STATUS "CMakeList end")