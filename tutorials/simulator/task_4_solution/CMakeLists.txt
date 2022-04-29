cmake_minimum_required(VERSION 3.11)

add_executable(tutorial-simulation_4_solution module_code.cpp)
target_link_libraries(tutorial-simulation_4_solution rofi::hal atoms)
