cmake_minimum_required(VERSION 3.21.0)
file(GLOB PROTO_FILES "*.proto")

add_library(proto OBJECT ${PROTO_FILES})

target_link_libraries(proto PUBLIC protobuf::libprotobuf)

target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
    TARGET proto
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}"
    PROTOS ${PROTO_FILES}
    # Default .pb.h and .pb.cc output path is ${CMAKE_CURRENT_BINARY_DIR}
)