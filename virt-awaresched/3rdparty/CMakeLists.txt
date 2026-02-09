add_subdirectory(googletest)
add_subdirectory(mockcpp)
add_subdirectory(secure_c)

add_custom_target(build_3rdparty
        COMMAND make build_googletest
        COMMAND make build_mockcpp
        COMMAND make build_securec
        WORKING_DIRECTORY ${PROJECT_TOP_BUILD_DIR}
)