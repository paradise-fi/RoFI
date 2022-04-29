cmake_minimum_required(VERSION 3.11)

add_executable(tutorial-simulation_3_solution module_code.cpp)
target_link_libraries(tutorial-simulation_3_solution rofi::hal atoms)
