#
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
#

file(GLOB_RECURSE TEST_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(test-optimizer ${TEST_SOURCE})

target_include_directories(test-optimizer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include"
        "${PROJECT_SOURCE_DIR}/3rdparty/mockcpp/include"
        "${libbpf_INCLUDE_DIRS}"
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/src/common"
        "${PROJECT_SOURCE_DIR}/3rdparty/googletest/googletest/include"
        "${PROJECT_SOURCE_DIR}/src/optimizer"
)

target_compile_options(test-optimizer PRIVATE -fno-access-control)

TARGET_LINK_LIBRARIES(test-optimizer ubs-opt-tuner-lib common-lib gtest ${PROJECT_SOURCE_DIR}/3rdparty/mockcpp/lib/libmockcpp.a ${libbpf_LIBRARIES}
        ${ELF_LIBRARIES}
        ${zlib_LIBRARIES}
        -lelf
        -lz
)