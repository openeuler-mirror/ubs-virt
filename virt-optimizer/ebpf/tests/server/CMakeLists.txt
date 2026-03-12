
file(GLOB_RECURSE TEST_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(test-server ${TEST_SOURCE} )

target_include_directories(test-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include" "${PROJECT_SOURCE_DIR}/3rdparty/mockcpp/include"
    "${libbpf_INCLUDE_DIRS}" "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/common" "${PROJECT_SOURCE_DIR}/3rdparty/googletest/googletest/include"
)

target_compile_options(test-server PRIVATE -fno-access-control)

TARGET_LINK_LIBRARIES(test-server ubs-opt-guard-lib common-lib gtest ${PROJECT_SOURCE_DIR}/3rdparty/mockcpp/lib/libmockcpp.a ${libbpf_LIBRARIES}
    ${ELF_LIBRARIES}
    ${zlib_LIBRARIES}
    -lelf
    -lz
)