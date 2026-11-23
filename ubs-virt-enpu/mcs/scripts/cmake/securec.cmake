if (TARGET c_sec)
    message(STATUS "Package c_sec has been found.")
    return()
endif()

find_path(
    C_SEC_BOUNDSCHECK_INCLUDE
    NAMES securec.h
)

find_library(
    C_SEC_BOUNDSCHECK_LIBRARY
    NAMES boundscheck
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(c_sec
    FOUND_VAR
        C_SEC_FOUND
    REQUIRED_VARS
        C_SEC_BOUNDSCHECK_INCLUDE
        C_SEC_BOUNDSCHECK_LIBRARY
)

if(C_SEC_FOUND)
    set(C_SEC_BOUNDSCHECK_INCLUDE_DIR ${C_SEC_BOUNDSCHECK_INCLUDE})
    get_filename_component(C_SEC_LIBRARY_DIR ${C_SEC_BOUNDSCHECK_LIBRARY} DIRECTORY)

    include(CMakePrintHelpers)
    message(STATUS "Variables in c_sec module:")
    cmake_print_variables(C_SEC_BOUNDSCHECK_INCLUDE)
    cmake_print_variables(C_SEC_LIBRARY_DIR)
    cmake_print_variables(C_SEC_BOUNDSCHECK_LIBRARY)

    add_library(c_sec_headers INTERFACE IMPORTED)
    target_include_directories(c_sec_headers INTERFACE ${C_SEC_BOUNDSCHECK_INCLUDE})

    add_library(c_sec SHARED IMPORTED)
    set_target_properties(c_sec PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${C_SEC_BOUNDSCHECK_INCLUDE}"
        IMPORTED_LOCATION             "${C_SEC_BOUNDSCHECK_LIBRARY}"
    )
else()
    message(FATAL_ERROR
            "libboundscheck not found! Please install the RPM package as shown in the example:\n"
            "  sudo rpm -ivh libboundscheck-v1.1.11-6.oe2403.aarch64.rpm\n"
            "Or via yum/dnf:\n"
            "  sudo yum install libboundscheck\n"
            "Or via source code of version 1.1.16:\n"
            "  https://gitcode.com/openeuler/libboundscheck.git"
    )
endif()