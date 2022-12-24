cmake_minimum_required(VERSION 3.11)


set (MSG_SRCS
    rofi/rofiCmd.proto
    rofi/rofiResp.proto
    rofi/rofiDescription.proto

    joint/jointCmd.proto
    joint/jointResp.proto
    joint/jointCapabilities.proto
    joint/jointSetPosWithSpeedCmd.proto
    joint/jointSetVelocityCmd.proto
    joint/jointSetTorqueCmd.proto

    connector/connectorAttachInfo.proto
    connector/connectorCmd.proto
    connector/connectorResp.proto
    connector/connectorPacket.proto
    connector/connectorState.proto

    distributor/distributorReq.proto
    distributor/distributorResp.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${MSG_SRCS})

add_library(rofisimMessages SHARED ${PROTO_SRCS})
target_link_libraries(rofisimMessages ${PROTOBUF_LIBRARY})
target_include_directories(rofisimMessages SYSTEM INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(rofisimMessages PRIVATE -Wno-conversion)
