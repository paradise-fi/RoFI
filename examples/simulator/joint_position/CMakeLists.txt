cmake_minimum_required(VERSION 3.11)

add_executable(joint_position joint_position.cpp)
target_link_libraries(joint_position rofiHal)
