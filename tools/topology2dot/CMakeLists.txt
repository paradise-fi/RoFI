cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-topology2dot topology2dot.cpp)
target_link_libraries(rofi-topology2dot PUBLIC original-configuration)

