cmake_minimum_required(VERSION 3.11)

add_executable(basicJointVelocity basicJointVelocity.cpp)
target_link_libraries(basicJointVelocity rofi::hal)
