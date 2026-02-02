if(TARGET securec)
    message(STATUS "securec already added.")
    return()
endif()

set(DEPS_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "Dependencies directory")
set(SECUREC_INSTALL_DIR "${DEPS_DIR}/securec")

if(EXISTS "${SECUREC_INSTALL_DIR}")
    message(STATUS "securec already built. Install directory found: ${SECUREC_INSTALL_DIR}")

    add_library(securec INTERFACE)
    target_include_directories(securec SYSTEM INTERFACE ${SECUREC_INSTALL_DIR}/include)
    target_link_libraries(securec INTERFACE ${SECUREC_INSTALL_DIR}/lib/libboundscheck.so)

    return()
else()
    message(STATUS "securec not found at: ${SECUREC_INSTALL_DIR}.")
endif()

include(FetchContent)
set(SECUREC_REPO "https://gitcode.com/openeuler/libboundscheck.git")
set(SECUREC_TAG "v1.1.16")

message(STATUS "Downloading securec ${SECUREC_TAG} from: ${SECUREC_REPO}.")
FetchContent_Declare(
    _securec_src
    GIT_REPOSITORY "${SECUREC_REPO}"
    GIT_TAG "${SECUREC_TAG}"
    GIT_SHALLOW TRUE
    QUIET
)
FetchContent_Populate(_securec_src)

set(SUB_BUILD "${_securec_src_SOURCE_DIR}")

execute_process(
    COMMAND make
    WORKING_DIRECTORY "${SUB_BUILD}"
    RESULT_VARIABLE res
)

file(REMOVE_RECURSE "${SECUREC_INSTALL_DIR}")
file(MAKE_DIRECTORY "${SECUREC_INSTALL_DIR}")
file(COPY "${SUB_BUILD}/include" DESTINATION "${SECUREC_INSTALL_DIR}")
file(COPY "${SUB_BUILD}/lib" DESTINATION "${SECUREC_INSTALL_DIR}")

add_library(securec INTERFACE)
target_include_directories(securec SYSTEM INTERFACE ${SECUREC_INSTALL_DIR}/include)
target_link_libraries(securec INTERFACE ${SECUREC_INSTALL_DIR}/lib/libboundscheck.so)
