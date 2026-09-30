set(VA_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
set(FT_REGRESSION_DIR ${CMAKE_CURRENT_LIST_DIR}/regression)
set(FT_SECURITY_DIR ${CMAKE_CURRENT_LIST_DIR}/security)

# Mutational fuzz harnesses.
add_executable(ft_va_cmd_deser ft_va_cmd_deser.cc)
target_include_directories(ft_va_cmd_deser PRIVATE ${VA_INCLUDE_DIR})
target_link_libraries(ft_va_cmd_deser PRIVATE ft_driver cmd_serialize)
set_target_properties(ft_va_cmd_deser PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FT_RUNTIME_DIR})

add_executable(ft_va_cli_parse ft_va_cli_parse.cc)
target_include_directories(ft_va_cli_parse PRIVATE ${VA_INCLUDE_DIR})
target_compile_options(ft_va_cli_parse PRIVATE -fno-access-control)
target_link_libraries(ft_va_cli_parse PRIVATE ft_driver cli_parse)
set_target_properties(ft_va_cli_parse PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FT_RUNTIME_DIR})

# Reuse the upstream tests without copying them into Fuzz. Only the new
# boundary cases live under Fuzz and are compiled into the isolated targets.
set(FT_PARSER_SOURCES
    ${PROJECT_SOURCE_DIR}/test/cli/parser/test_vas_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/test/cli/parser/test_vas_cli_process.cpp
    ${PROJECT_SOURCE_DIR}/test/cli/parser/test_vas_cli_res_echo.cpp
    ${FT_REGRESSION_DIR}/parser/test_vas_cli_parse_extra.cc
)
ft_add_gtest(VirtAwareSched_parser_ut
    LABEL parser
    SOURCES ${FT_PARSER_SOURCES}
    LIBRARIES cli_parse acquire vas_ctl_register cluster_sched logger util cmd_serialize socket_client
)

set(FT_SERIALIZE_SOURCES
    ${PROJECT_SOURCE_DIR}/test/cli/opt_serialize/test_cmd_serialize.cpp
)
ft_add_gtest(VirtAwareSched_cmd_serialize_ut
    LABEL cmd_serialize
    SOURCES ${FT_SERIALIZE_SOURCES}
    LIBRARIES cmd_serialize
    COMPILE_OPTIONS
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-maybe-uninitialized
        -Wno-conversion
        -Wno-float-conversion
)

set(FT_SOCKET_SERVER_SOURCES
    ${PROJECT_SOURCE_DIR}/test/cli/socket_server/test_socket_server.cpp
    ${FT_REGRESSION_DIR}/socket_server/test_socket_server_extra.cc
)
ft_add_gtest(VirtAwareSched_socket_server_ut
    LABEL socket_server
    SOURCES ${FT_SOCKET_SERVER_SOURCES}
    LIBRARIES socket_server
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/vasd/security
    COMPILE_OPTIONS
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-maybe-uninitialized
        -Wno-conversion
        -Wno-float-conversion
)

set(FT_SOCKET_CLIENT_SOURCES
    ${PROJECT_SOURCE_DIR}/test/cli/socket_client/test_socket_client.cpp
)
ft_add_gtest(VirtAwareSched_socket_client_ut
    LABEL socket_client
    SOURCES ${FT_SOCKET_CLIENT_SOURCES}
    LIBRARIES socket_client
    COMPILE_OPTIONS
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-maybe-uninitialized
        -Wno-conversion
        -Wno-float-conversion
)

set(FT_API_SOURCES
    ${PROJECT_SOURCE_DIR}/test/vasd/api/test_api.cpp
    ${FT_REGRESSION_DIR}/api/test_api_extra.cc
)
ft_add_gtest(VirtAwareSched_api_ut
    LABEL api
    SOURCES ${FT_API_SOURCES}
    LIBRARIES api cmd_serialize vas_daemon_register
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/vasd/cluster_sched
    COMPILE_OPTIONS
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-maybe-uninitialized
        -Wno-conversion
        -Wno-float-conversion
)

file(GLOB_RECURSE FT_SECURITY_SOURCES CONFIGURE_DEPENDS
    ${FT_SECURITY_DIR}/*.cc
    ${FT_SECURITY_DIR}/*.h
)
ft_add_gtest(VirtAwareSched_security_ut
    LABEL security
    SOURCES ${FT_SECURITY_SOURCES}
    LIBRARIES
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
    INCLUDE_DIRECTORIES
        ${FT_SECURITY_DIR}
        ${PROJECT_SOURCE_DIR}/test/util
        ${PROJECT_SOURCE_DIR}/src/vasd/security
)

add_custom_target(fuzz_tests
    DEPENDS
        ft_va_cmd_deser
        ft_va_cli_parse
        VirtAwareSched_parser_ut
        VirtAwareSched_cmd_serialize_ut
        VirtAwareSched_socket_server_ut
        VirtAwareSched_socket_client_ut
        VirtAwareSched_api_ut
        VirtAwareSched_security_ut
)
