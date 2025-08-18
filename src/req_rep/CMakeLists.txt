cmake_minimum_required(VERSION 3.14)

project(cppzmq_request_reply)

add_executable(cppzmq_request ${CMAKE_CURRENT_SOURCE_DIR}/request.cc)
target_link_libraries(cppzmq_request PRIVATE ${lib_cppzmq} ${lib_protobuf} ${sim_msg} ${lib_log} ${lib_sys})

add_executable(cppzmq_reply ${CMAKE_CURRENT_SOURCE_DIR}/reply.cc)
target_link_libraries(cppzmq_reply PRIVATE ${lib_cppzmq} ${lib_protobuf} ${sim_msg} ${lib_log} ${lib_sys})