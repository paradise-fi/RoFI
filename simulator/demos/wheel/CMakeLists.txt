cmake_minimum_required(VERSION 3.11)

add_executable(wheel wheel.cpp)
target_link_libraries(wheel rofiHal)

create_world(wheel.world wheel.in)
