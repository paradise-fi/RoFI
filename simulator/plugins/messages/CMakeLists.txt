cmake_minimum_required(VERSION 3.10)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS}")

find_package(Protobuf REQUIRED)
# Include Gazebo messages
set(PROTOBUF_IMPORT_DIRS)
foreach(ITR ${GAZEBO_INCLUDE_DIRS})
    if(ITR MATCHES ".*gazebo-[0-9.]+$")
        set(PROTOBUF_IMPORT_DIRS "${ITR}/gazebo/msgs/proto")
    endif()
endforeach()

set (msgs
    rofi/rofiCmd.proto
    rofi/rofiResp.proto
    rofi/rofiDescription.proto

    joint/jointCmd.proto
    joint/jointResp.proto
    joint/jointSetPosWithSpeedCmd.proto
    joint/jointSetVelocityCmd.proto
    joint/jointSetTorqueCmd.proto

    connector/connectorAttachInfo.proto
    connector/connectorCmd.proto
    connector/connectorResp.proto
    connector/connectorPacket.proto
    connector/connectorState.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${msgs})

add_library(rofiMessages SHARED ${PROTO_SRCS})
target_link_libraries(rofiMessages ${PROTOBUF_LIBRARY})
target_include_directories(rofiMessages INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
