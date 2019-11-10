cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

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
    messages/rofi/rofiCmd.proto
    messages/rofi/rofiResp.proto

    messages/joint/jointCmd.proto
    messages/joint/jointResp.proto
    messages/joint/jointSetPosWithSpeedCmd.proto
    messages/joint/jointSetVelocityCmd.proto
    messages/joint/jointSetTorqueCmd.proto

    messages/connector/connectorCmd.proto
    messages/connector/connectorResp.proto
    messages/connector/connectorPacket.proto
    messages/connector/connectorPowerState.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${msgs})

add_library(universalModulePluginMsgs SHARED ${PROTO_SRCS})
target_link_libraries(universalModulePluginMsgs ${PROTOBUF_LIBRARY})
target_include_directories(universalModulePluginMsgs INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

add_library(universalModulePlugin SHARED universalModulePlugin.cpp)
target_link_libraries(universalModulePlugin ${GAZEBO_LIBRARIES} universalModulePluginMsgs)
