cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(kinematics src/fReconfig.cpp)
target_include_directories(kinematics PUBLIC include)
target_link_libraries(kinematics PUBLIC configuration legacy-configuration atoms)
