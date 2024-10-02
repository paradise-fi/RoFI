cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-freconfig reconfiguration.cpp)
target_include_directories(rofi-freconfig INTERFACE .)
target_link_libraries(rofi-freconfig PRIVATE kinematics legacy-configuration dimcli)
