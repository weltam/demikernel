# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

cmake_minimum_required (VERSION 3.5.1)
project (SimpleKeyValueStore C CXX)

#set(RUNTIME_OUTPUT_DIRECTORY "execs")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS
  program_options chrono context system coroutine)
find_package(Protobuf REQUIRED)
find_package(CapnProto REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${CapnProto_INCLUDE_DIR})

set(KV_APPS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

protobuf_generate_cpp(SRC_KV_MSG_PROTO HDR_KV_MSG_PROTO
    ${KV_APPS_DIR}/stress.proto)
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${KV_APPS_DIR}/stress.capnp)

include_directories(${PROJECT_BINARY_DIR})

find_program(FLATBUFFERS_COMPILER flatc)

set(flatc_language "--cpp")
set(FLATC_OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/stress_generated.h)

add_custom_command(OUTPUT ${FLATC_OUTPUT}
                COMMAND ${FLATBUFFERS_COMPILER}
                ARGS ${flatc_language}
                ARGS ${KV_APPS_DIR}/stress.fbs)

add_custom_target(flatc_header_kv ALL DEPENDS ${FLATC_OUTPUT})

set(TCP_KV_SERVER_SOURCES ${KV_APPS_DIR}/dmtr_tcp_server.cc
    ${SRC_KV_MSG_PROTO} ${HDR_KV_MSG_PROTO} ${KV_APPS_DIR}/protobuf.cc
    ${CAPNP_SRCS} ${KV_APPS_DIR}/flatbuffers.cc ${KV_APPS_DIR}/capnproto.cc
    ${KV_APPS_DIR}/kv.cc ${KV_APPS_DIR}/handcrafted.cc)
set(TCP_KV_CLIENT_SOURCES ${KV_APPS_DIR}/dmtr_tcp_client.cc 
    ${SRC_KV_MSG_PROTO} ${HDR_KV_MSG_PROTO} ${KV_APPS_DIR}/protobuf.cc
    ${CAPNP_SRCS} ${KV_APPS_DIR}/flatbuffers.cc ${KV_APPS_DIR}/capnproto.cc
    ${KV_APPS_DIR}/kv.cc ${KV_APPS_DIR}/handcrafted.cc)

# POSIX TCP server
add_executable(dmtr-posix-kv-server ${TCP_KV_SERVER_SOURCES})
target_link_libraries(dmtr-posix-kv-server dmtr-libos-posix yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp) 

# POSIX TCP client
add_executable(dmtr-posix-kv-client ${TCP_KV_CLIENT_SOURCES})
target_link_libraries(dmtr-posix-kv-client dmtr-libos-posix yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp)

# POSIX TCP client & server (works with storage too)
add_custom_target(dmtr-posix-kv)
add_dependencies(dmtr-posix-kv  dmtr-posix-kv-server dmtr-posix-kv-client
    flatc_header_kv)

# LWIP TCP server
add_executable(dmtr-lwip-kv-server ${TCP_KV_SERVER_SOURCES})
target_link_libraries(dmtr-lwip-kv-server dmtr-libos-lwip yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp)

# LWIP TCP client
add_executable(dmtr-lwip-kv-client  ${TCP_KV_CLIENT_SOURCES})
target_link_libraries(dmtr-lwip-kv-client dmtr-libos-lwip yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp)

# LWIP TCP client & server
add_custom_target(dmtr-lwip-kv)
add_dependencies(dmtr-lwip-kv dmtr-lwip-kv-server dmtr-lwip-kv-client flatc_header_kv)

# RDMA server
add_executable(dmtr-rdma-kv-server ${TCP_KV_SERVER_SOURCES})
target_link_libraries(dmtr-rdma-kv-server dmtr-libos-rdma rdmacm ibverbs yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp)

# RDMA client
add_executable(dmtr-rdma-kv-client ${TCP_KV_CLIENT_SOURCES})
target_link_libraries(dmtr-rdma-kv-client dmtr-libos-rdma rdmacm ibverbs yaml-cpp
    boost_program_options ${Protobuf_LIBRARIES} CapnProto::capnp)

# RDMA client & server
add_custom_target(dmtr-rdma-kv)
add_dependencies(dmtr-rdma-kv dmtr-rdma-kv-server dmtr-rdma-kv-client flatc_header_kv)
