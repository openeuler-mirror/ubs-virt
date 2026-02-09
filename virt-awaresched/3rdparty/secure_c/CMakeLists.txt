if (SOURCE_COMPILING)
    set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/source)

    add_custom_target(build_securec
            COMMAND mkdir -p ${PROJECT_DEPS_DIR}/securec
            COMMAND rm -rf ${PROJECT_DEPS_DIR}/securec/*
            COMMAND make -C src/
            COMMAND find obj/ -name "*.o" | sort | xargs ar rcs lib/libsecurec.a
            COMMAND cp -rv include/ ${PROJECT_DEPS_DIR}/securec/
            COMMAND cp -rv lib/ ${PROJECT_DEPS_DIR}/securec/
            COMMAND rm -rf lib/ obj/
            WORKING_DIRECTORY ${SOURCE_DIR}
            COMMENT "build secure_c from source code"
            VERBATIM
    )

    if (DEFINED ENV{SYNC_DEV_CMC} AND "$ENV{SYNC_DEV_CMC}" STREQUAL "1")
        add_custom_command(
                TARGET build_securec
                POST_BUILD
                COMMAND bash ${PROJECT_SOURCE_DIR}/scripts/build/upload_dep.sh securec aarch64
                COMMENT "upload securec_aarch64.tar.gz to BeiMing 24.4.RC1.B099"
                VERBATIM
        )
    endif ()
else ()
    download_dep(securec)
endif ()

add_library(securec INTERFACE)
target_include_directories(securec SYSTEM INTERFACE ${PROJECT_DEPS_DIR}/securec/include)
target_link_libraries(securec INTERFACE ${PROJECT_DEPS_DIR}/securec/lib/libsecurec.so)

install_lib(securec)