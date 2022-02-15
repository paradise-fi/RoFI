cmake_minimum_required(VERSION 3.11)

add_executable(doubleWheel doubleWheel.cpp)
target_link_libraries(doubleWheel rofi::hal)

add_gazebo_world(doubleWheel.world doubleWheel.in)
