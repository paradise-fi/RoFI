cmake_minimum_required(VERSION 3.6)
project(rofi)


add_library(kinematics kinematics.cpp calculations.hpp)
target_include_directories(kinematics PUBLIC .)
target_link_libraries(kinematics PRIVATE legacy-configuration)
