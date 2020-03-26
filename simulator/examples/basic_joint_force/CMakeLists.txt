cmake_minimum_required(VERSION 3.10)

add_executable(basic_joint_force basic_joint_force.cpp)
target_link_libraries(basic_joint_force rofiHal)
