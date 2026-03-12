find_package(Threads REQUIRED)
file(GLOB_RECURSE COMMON_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(common-lib
    ${COMMON_SOURCE}
)

target_link_libraries(common-lib PRIVATE Threads::Threads stdc++fs
    -lgcov
)

target_include_directories(common-lib PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include"
)
