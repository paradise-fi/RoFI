cmake_minimum_required(VERSION 3.11)

add_executable(basicJointForce basicJointForce.cpp)
target_link_libraries(basicJointForce rofi::hal)
