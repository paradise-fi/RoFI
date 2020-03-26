cmake_minimum_required(VERSION 3.10)

add_executable(jointPositionDemo joint_position_demo.cpp)
target_link_libraries(jointPositionDemo rofiHal)
