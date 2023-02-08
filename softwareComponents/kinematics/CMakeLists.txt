cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(kinematics fReconfig.cpp)
target_include_directories(kinematics INTERFACE .)
target_link_libraries(kinematics PRIVATE legacy-configuration atoms)

add_library(aabb include/aabb.hpp)
target_link_libraries(aabb configuration)

add_executable(test-aabb test/aabb.cpp)
target_include_directories(test-aabb PRIVATE include)
target_link_libraries(test-aabb PRIVATE Catch2WithMain aabb)
