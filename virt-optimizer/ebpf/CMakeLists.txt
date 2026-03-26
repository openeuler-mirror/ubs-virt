cmake_minimum_required(VERSION 3.14.1)
project(UBS_OPT VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -O3
    -s
    -fstack-protector-all
    -fstack-protector-strong
    -Wextra
    -Wfloat-equal
    -fno-common
    -ftrapv
    -fPIC
    -D_FORTIFY_SOURCE=2
)

add_link_options(
    -Wl,-z,relro,-z,now
    -Wl,-z,noexecstack
    -pie
)

message(STATUS "fetch rapidjson")
include(FetchContent)
FetchContent_Declare(
        rapidjson
        GIT_REPOSITORY https://gitcode.com/GitHub_Trending/ra/rapidjson.git
        GIT_TAG master
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rapidjson
        GIT_SUBMODULES ""
)
FetchContent_Populate(rapidjson)

IF(${CMAKE_BUILD_TYPE} MATCHES "Release")
    add_subdirectory(src)
ENDIF()

IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(UTCOV_TEST
        -lgcov
    )
    add_subdirectory(src)
    add_subdirectory(3rdparty/googletest)
    add_subdirectory(tests)
ENDIF()
