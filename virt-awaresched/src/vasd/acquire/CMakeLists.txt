file(GLOB_RECURSE ACQUIRE_CPP "*.cpp")
FIND_INCLUDE_DIR(ACQUIRE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(acquire STATIC ${ACQUIRE_CPP})
target_include_directories(acquire PUBLIC ${ACQUIRE_INCLUDE})
target_link_libraries(acquire PRIVATE util logger conf libvirt_module vas_daemon_register)
