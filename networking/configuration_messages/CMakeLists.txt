cmake_minimum_required(VERSION 3.11)

find_package(Protobuf REQUIRED)
# Include Gazebo messages
set(PROTOBUF_IMPORT_DIRS)
foreach(ITR ${GAZEBO_INCLUDE_DIRS})
    if(ITR MATCHES ".*gazebo-[0-9.]+$")
        set(PROTOBUF_IMPORT_DIRS "${ITR}/gazebo/msgs/proto")
    endif()
endforeach()

set (msgs
    common.proto
    component.proto
    joint.proto
    module.proto
    rofibot.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${msgs})

add_library(rofiConfigurationMessages SHARED ${PROTO_SRCS})
target_link_libraries(rofiConfigurationMessages ${PROTOBUF_LIBRARY})
target_include_directories(rofiConfigurationMessages INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(rofiConfigurationMessages PROPERTIES COMPILE_FLAGS "-Wno-all -Wno-extra -Wno-pedantic")
