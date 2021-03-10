cmake_minimum_required(VERSION 3.11)

add_executable(double_wheel double_wheel.cpp)
target_link_libraries(double_wheel rofiHal)

create_world(double_wheel.world double_wheel.in)
