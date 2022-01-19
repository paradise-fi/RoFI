cmake_minimum_required(VERSION 3.11)

set (MSG_SRCS
    simplesim_settings_cmd.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${MSG_SRCS})

add_library(simplesimConfigMsgs SHARED ${PROTO_SRCS})
target_link_libraries(simplesimConfigMsgs ${PROTOBUF_LIBRARY})
target_include_directories(simplesimConfigMsgs SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
