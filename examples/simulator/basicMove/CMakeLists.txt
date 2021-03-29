cmake_minimum_required(VERSION 3.11)

add_executable(basicMove basicMove.cpp)
target_link_libraries(basicMove rofiHal)

add_gazebo_world(basicMove.world basicMove.in)
