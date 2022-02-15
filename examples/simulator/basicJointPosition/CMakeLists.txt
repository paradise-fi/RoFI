cmake_minimum_required(VERSION 3.11)

add_executable(basicJointPosition basicJointPosition.cpp)
target_link_libraries(basicJointPosition rofi::hal)
