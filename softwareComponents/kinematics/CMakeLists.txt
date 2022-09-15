cmake_minimum_required(VERSION 3.6)
project(rofi)


add_library(kinematics fReconfig.cpp)
target_include_directories(kinematics INTERFACE .)
target_link_libraries(kinematics PRIVATE legacy-configuration atoms)
