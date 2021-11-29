cmake_minimum_required(VERSION 3.6)
project(rofi)


add_executable(rofi-ik generator.cpp)
target_include_directories(rofi-ik INTERFACE .)
target_link_libraries(rofi-ik PRIVATE kinematics legacy-configuration)
