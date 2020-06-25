cmake_minimum_required(VERSION 3.11)

add_executable(jointPositionDemo joint_position_demo.cpp)
target_link_libraries(jointPositionDemo rofiHal)
