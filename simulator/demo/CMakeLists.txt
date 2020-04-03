cmake_minimum_required(VERSION 3.10)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")


add_subdirectory(command_line)
add_subdirectory(joint_position)
add_subdirectory(packet)

add_subdirectory(roficom_command_line)
