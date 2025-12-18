include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/server
        ${CMAKE_CURRENT_SOURCE_DIR}/service
        ${CMAKE_CURRENT_SOURCE_DIR}/serde
)

set(SRC_FILES
        main.cpp
)

add_executable(ubs_virt_ovs ${SRC_FILES})