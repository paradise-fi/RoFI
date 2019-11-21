cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

add_executable(command_line_demo command_line_demo.cpp rofi_hal_sim.cpp)
target_link_libraries(command_line_demo rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(joint_move_demo joint_move_demo.cpp rofi_hal_sim.cpp)
target_link_libraries(joint_move_demo rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(packet_demo packet_demo.cpp rofi_hal_sim.cpp )
target_link_libraries(packet_demo rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
