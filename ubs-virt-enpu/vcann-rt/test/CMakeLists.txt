add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)

include(${PROJECT_SOURCE_DIR}/scripts/cmake/gtest.cmake)
include(${PROJECT_SOURCE_DIR}/scripts/cmake/mockcpp.cmake)
include(${PROJECT_SOURCE_DIR}/scripts/cmake/securec.cmake)

set(VNPU_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/core_limiter.c
    ${PROJECT_SOURCE_DIR}/src/mem_limiter.c
    ${PROJECT_SOURCE_DIR}/src/npu_manager.c
    ${PROJECT_SOURCE_DIR}/src/log.c
    ${PROJECT_SOURCE_DIR}/src/ascend/device.c
    ${PROJECT_SOURCE_DIR}/src/ascend/event.c
    ${PROJECT_SOURCE_DIR}/src/ascend/graph.c
    ${PROJECT_SOURCE_DIR}/src/ascend/hook.c
    ${PROJECT_SOURCE_DIR}/src/ascend/kernel.c
    ${PROJECT_SOURCE_DIR}/src/ascend/memory.c
    ${PROJECT_SOURCE_DIR}/src/ascend/task.c
    ${PROJECT_SOURCE_DIR}/src/utils/config.c
    ${PROJECT_SOURCE_DIR}/src/utils/dcmi_wrapper.c
    ${PROJECT_SOURCE_DIR}/src/utils/hash_map.c
    ${PROJECT_SOURCE_DIR}/src/utils/utils.c
)

set(VNPU_TESTCASE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/testcase/demo_test.cpp
)

set(VNPU_STUB_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/stub/runtime_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stub/dcmi_stub.c
)

add_executable(vnpu_test
    ${VNPU_SRC_FILES}
    ${VNPU_TESTCASE_FILES}
    ${VNPU_STUB_FILES}
)

target_include_directories(vnpu_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/stub
)

target_link_libraries(vnpu_test PRIVATE
    gtest
    gtest_main
    mockcpp
    pthread
    rt
    dl
    securec
)

target_compile_definitions(vnpu_test
    PRIVATE
    _GNU_SOURCE
)

target_compile_options(vnpu_test PRIVATE
    -Wall -Wextra -Wformat-nonliteral -Wformat-security -Wformat-y2k -Wfloat-equal
    -std=c++14
    -fPIC
    -fstack-protector-strong
    -fvisibility=hidden
    -fno-common
)

target_link_options(vnpu_test PRIVATE
    -Wl,-z,-relro,-z,now,-z,noexecstack
    -s
)

if(ENABLE_COVERAGE)
    add_compile_options(-fprofile-arcs -ftest-coverage)
    target_compile_options(vnpu_test PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(vnpu_test PRIVATE -fprofile-arcs -ftest-coverage)
endif()
