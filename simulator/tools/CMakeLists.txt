cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS}")

add_executable(command_line_demo command_line_demo.cpp rofi_hal_sim.cpp)
target_link_libraries(command_line_demo universalModulePluginMsgs ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(sim_demo rofi_hal_sim_demo.cpp rofi_hal_sim.cpp)
target_link_libraries(sim_demo universalModulePluginMsgs ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
