add_library(ubs-virt-ovs-core STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/services/urma/urma_service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/server/connection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/server/server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/server/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/core/service_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ubse/client_library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ubse/urma_utility.cpp
)

target_include_directories(ubs-virt-ovs-core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/server
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/code
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/core
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/protocol
        ${CMAKE_CURRENT_SOURCE_DIR}/services
        ${CMAKE_CURRENT_SOURCE_DIR}/services/urma
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CMAKE_CURRENT_SOURCE_DIR}/serde
)

add_executable(ubs-virt-ovs
        main.cpp
)

target_link_libraries(ubs-virt-ovs
        PRIVATE ubs-virt-ovs-core
)

if (BUILD_TESTS)
    SET(TEST_BIN_DIR "${CMAKE_BINARY_DIR}/test")
    target_compile_options(ubs-virt-ovs-core PRIVATE --coverage -O0 -g)
    target_link_libraries(ubs-virt-ovs-core PUBLIC --coverage)
    set_target_properties(ubs-virt-ovs-core PROPERTIES OUTPUT_NAME "ubs-virt-ovs-core")
    if (COVERAGE)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${TEST_BIN_DIR}/virt_ut --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/coverage/test_detail.xml
            COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --directory ${CMAKE_CURRENT_BINARY_DIR} --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' --output-file coverage.info
            COMMAND genhtml coverage.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif ()

endif ()