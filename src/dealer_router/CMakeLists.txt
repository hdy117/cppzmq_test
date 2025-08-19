cmake_minimum_required(VERSION 3.14)

project(dealer_router_test)

add_executable(dealer_router ${CMAKE_CURRENT_SOURCE_DIR}/dealer_router.cc)
target_link_libraries(dealer_router PRIVATE ${lib_cppzmq} ${lib_protobuf} ${sim_msg} ${lib_log} ${lib_sys})

add_executable(dealer_router_poll ${CMAKE_CURRENT_SOURCE_DIR}/dealer_router_poll.cc)
target_link_libraries(dealer_router_poll PRIVATE ${lib_cppzmq} ${lib_protobuf} ${sim_msg} ${lib_log} ${lib_sys})