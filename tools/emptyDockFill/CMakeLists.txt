cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-emptyDockFill emptyDockFill.cpp)
target_link_libraries(rofi-emptyDockFill PUBLIC configuration)

