cmake_minimum_required(VERSION 3.15)
project(vcann-runtime)
set(CMAKE_SKIP_RPATH TRUE)
message(STATUS "CMakeList start ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SRC_LIST
    src/*.c
    src/ascend/*.c
    src/tools/*.c
    src/utils/*.c
)
list(FILTER SRC_LIST EXCLUDE REGEX ".*/src/tools/monitor\\.c$")
list(APPEND SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/allocation_id.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/enpu_dcmi.c)

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

set(LINK_LIBS pthread dl rt)
set(RUNTIME_ABI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime_abi")

message(STATUS "CMakeList build start")
add_library(vruntime SHARED src/ascend/hook.c ${SRC_LIST})
add_custom_command(
    TARGET vruntime POST_BUILD
    COMMAND chmod 400 $<TARGET_FILE:vruntime>
)

target_link_options(vruntime PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-z,defs
    -s
)

target_include_directories(vruntime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${RUNTIME_ABI_INCLUDE_DIR}
)
target_link_libraries(vruntime PRIVATE ${LINK_LIBS} c_sec)
target_compile_options(vruntime PRIVATE ${SECURE_COMPILE_OPTIONS})
target_compile_definitions(vruntime PRIVATE _GLIBCXX_USE_CXX11_ABI=0)

add_executable(enpu-monitor src/tools/monitor.c ${SRC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/vnpu_metrics_reader.c)
add_custom_command(
    TARGET enpu-monitor POST_BUILD
    COMMAND chmod 500 $<TARGET_FILE:enpu-monitor>
)

target_link_options(enpu-monitor PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

target_include_directories(enpu-monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${RUNTIME_ABI_INCLUDE_DIR}
)
target_link_libraries(enpu-monitor PRIVATE ${LINK_LIBS} c_sec)
target_compile_options(enpu-monitor PRIVATE ${SECURE_COMPILE_OPTIONS})
target_compile_definitions(enpu-monitor PRIVATE _GLIBCXX_USE_CXX11_ABI=0)

option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

message(STATUS "CMakeList end")
