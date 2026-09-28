# DT Fuzz and security regression targets.
#
# This file is loaded through CMAKE_PROJECT_INCLUDE. Keeping the entry point
# here allows all test integration to remain under test/Fuzz without changing
# the production or existing test CMake files.

# CMAKE_PROJECT_INCLUDE is inherited by FetchContent subprojects. Only the
# top-level VirtAwareSched project owns these options and test targets.
if (NOT PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    return()
endif ()

option(FT_COVERAGE "Enable gcov support for DT Fuzz child processes" OFF)
option(FT_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if (FT_COVERAGE)
    set(ENABLE_COVERAGE ON CACHE BOOL "Use gcov" FORCE)
endif ()

if (FT_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif ()

if (TARGET ft_driver)
    return()
endif ()

include(${PROJECT_SOURCE_DIR}/cmake/gtest.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/mockcpp.cmake)
include(GoogleTest)

set(FT_TEST_MAIN ${PROJECT_SOURCE_DIR}/test/test_main.cpp)
set(FT_RUNTIME_DIR ${CMAKE_BINARY_DIR}/bin)

function(ft_add_gtest target)
    set(options)
    set(one_value_args LABEL)
    set(multi_value_args SOURCES LIBRARIES INCLUDE_DIRECTORIES COMPILE_OPTIONS)
    cmake_parse_arguments(FT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    add_executable(${target} EXCLUDE_FROM_ALL ${FT_SOURCES} ${FT_TEST_MAIN})
    target_compile_options(${target} PRIVATE -fno-access-control ${DEBUG_FLAGS} -w ${FT_COMPILE_OPTIONS})
    target_include_directories(${target} PRIVATE
        ${PROJECT_SOURCE_DIR}/src/include
        ${PROJECT_SOURCE_DIR}/src/util
        ${FT_INCLUDE_DIRECTORIES}
    )
    target_link_libraries(${target} PRIVATE gtest pthread gcov mockcpp ${FT_LIBRARIES})
    set_target_properties(${target} PROPERTIES
        LINK_FLAGS "-Wl,--as-needed"
        RUNTIME_OUTPUT_DIRECTORY ${FT_RUNTIME_DIR}
    )
    gtest_discover_tests(${target} PROPERTIES LABELS "${FT_LABEL}" DISCOVERY_MODE PRE_TEST)
endfunction()

set(FT_ROOT ${PROJECT_SOURCE_DIR}/test/Fuzz)
add_subdirectory(${FT_ROOT}/driver ${CMAKE_BINARY_DIR}/Fuzz/driver)
add_subdirectory(${FT_ROOT}/virt_awaresched ${CMAKE_BINARY_DIR}/Fuzz/virt_awaresched)
