set(OVS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${OVS_DIR}/..)

add_executable(ubs-virt-ovs
        ${OVS_DIR}/main.cpp
        ${OVS_DIR}/services
        ${OVS_DIR}/services/urma/urma_service.cpp
        ${OVS_DIR}/ipc/server/connection.cpp
        ${OVS_DIR}/ipc/server/server.cpp
        ${OVS_DIR}/ipc/server/thread_pool.cpp
        ${OVS_DIR}/ipc/server/dispatcher.cpp
        ${OVS_DIR}/ipc/core/service_base.cpp
        ${SRC_DIR}/logger/logger.cpp
)

target_include_directories(ubs-virt-ovs
    PRIVATE
        ${OVS_DIR}
        ${OVS_DIR}/ipc
        ${OVS_DIR}/ipc/server
        ${OVS_DIR}/ipc/code
        ${OVS_DIR}/ipc/core
        ${OVS_DIR}/ipc/protocol
        ${OVS_DIR}/services
        ${OVS_DIR}/services/urma
        ${SRC_DIR}/logger
        ${SRC_DIR}/serde
)