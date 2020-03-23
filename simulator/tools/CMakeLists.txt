cmake_minimum_required(VERSION 3.10)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

add_library(rofiHal SHARED rofi_hal_sim.cpp)
target_link_libraries(rofiHal rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofiHal INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(tests)

add_executable(command_line_demo command_line_demo.cpp)
target_link_libraries(command_line_demo rofiHal)

add_executable(joint_move_demo joint_move_demo.cpp)
target_link_libraries(joint_move_demo rofiHal)

add_executable(packet_demo packet_demo.cpp)
target_link_libraries(packet_demo rofiHal)
