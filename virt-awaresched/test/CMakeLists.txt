include_directories(${PROJECT_DEPS_DIR}/googletest/include)
include_directories(${PROJECT_DEPS_DIR}/mockcpp/include)
link_directories(${PROJECT_DEPS_DIR}/googletest/lib64)
link_directories(${PROJECT_DEPS_DIR}/mockcpp/lib)
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

set(UT_BINARY ${CMAKE_PROJECT_NAME}_ut)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/test)
message(STATUS "CMAKE_BINARY_DIR  ${CMAKE_BINARY_DIR}")
message(STATUS "test dir ${TEST_DIR}")

configure_file (
        "${PROJECT_SOURCE_DIR}/test/config.h.in"
        "${CMAKE_BINARY_DIR}/include/rack_test/config.h"
)

# UT Document
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)

add_executable(${UT_BINARY} EXCLUDE_FROM_ALL ${TEST_SOURCES})

# Breaking control
target_compile_options(${UT_BINARY} PRIVATE -fno-access-control ${DEBUG_FLAGS} -w)
# Test Piece Library
target_link_libraries(${UT_BINARY} PUBLIC gtest pthread gcov mockcpp)
# Business Static Library
target_link_libraries(${UT_BINARY} PUBLIC
        cmd_serialize
        cli_parse
        socket_server
        socket_client
        acquire
        api
        vas_daemon_register
        util
        logger
        cluster_sched
        looper
        vas_ctl_register
        security
)
set_target_properties(${UT_BINARY} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/test)

add_dependencies(${UT_BINARY} build_googletest)
add_dependencies(${UT_BINARY} build_mockcpp)
add_custom_target(build_test COMMAND export "VIRT_CURL_PATH=/usr/lib64"
        && ${PROJECT_SOURCE_DIR}/test/${UT_BINARY} --gtest_output=xml:${PROJECT_SOURCE_DIR}/build/test_detail.xml WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test)
add_dependencies(build_test ${UT_BINARY})

include(GoogleTest)
gtest_add_tests(TARGET ${UT_BINARY})

add_subdirectory(vasd)
add_subdirectory(cli)