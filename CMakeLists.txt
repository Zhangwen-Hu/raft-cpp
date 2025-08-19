cmake_minimum_required(VERSION 3.16)
project(raft-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Threads REQUIRED)

# Proto files
set(PROTO_FILES
    proto/raft.proto
)

# Generate protobuf and gRPC files
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(proto_file ${PROTO_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WE)
    get_filename_component(proto_path ${proto_file} PATH)
    
    set(proto_src "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    set(grpc_src "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
    set(grpc_hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")
    
    list(APPEND PROTO_SRCS ${proto_src})
    list(APPEND PROTO_HDRS ${proto_hdr})
    list(APPEND GRPC_SRCS ${grpc_src})
    list(APPEND GRPC_HDRS ${grpc_hdr})
    
    add_custom_command(
        OUTPUT ${proto_src} ${proto_hdr} ${grpc_src} ${grpc_hdr}
        COMMAND protobuf::protoc
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
             --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
             -I "${CMAKE_CURRENT_SOURCE_DIR}"
             --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
             "${CMAKE_CURRENT_SOURCE_DIR}/${proto_file}"
        DEPENDS ${proto_file}
        COMMENT "Running gRPC C++ protocol buffer compiler on ${proto_file}"
        VERBATIM
    )
endforeach()

# Include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(RAFT_SOURCES
    src/raft_node.cpp
    src/raft_server.cpp
    src/log_entry.cpp
    src/persistent_state.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# Raft library
add_library(raft_lib ${RAFT_SOURCES})
target_link_libraries(raft_lib
    gRPC::grpc++
    protobuf::libprotobuf
    Threads::Threads
)

# Raft server executable
add_executable(raft_server src/main.cpp)
target_link_libraries(raft_server raft_lib)

# Test executable
add_executable(raft_test src/test_cluster.cpp)
target_link_libraries(raft_test raft_lib)

# Install
install(TARGETS raft_server raft_test
    RUNTIME DESTINATION bin
) 