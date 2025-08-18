cmake_minimum_required(VERSION 3.14)

project(cppzmq_test)

# submodules
set(scheduler scheduler)
set(module_service module_service)
set(sim_msg sim_msg)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/message)
include_directories(${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    find_package(cppzmq CONFIG REQUIRED)
    find_package(glog CONFIG REQUIRED)
    find_package(protobuf CONFIG REQUIRED)

    set(lib_sys shlwapi)
    set(lib_log glog::glog gflags::gflags)
    set(lib_cppzmq cppzmq)
    set(lib_protobuf protobuf::libprotobuf)

    include_directories(D:/work/git/vcpkg/installed/x64-windows/include)
else()
    include_directories(/usr/local/include /usr/include)
    link_directories(/usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu)

    set(lib_sys pthread)
    set(lib_log glog gflags)
    set(lib_protobuf protobuf)
endif()

aux_source_directory(${CMAKE_SOURCE_DIR}/message sim_msg_files)

add_library(${sim_msg} STATIC ${sim_msg_files})

add_subdirectory(${CMAKE_SOURCE_DIR}/src/req_rep)