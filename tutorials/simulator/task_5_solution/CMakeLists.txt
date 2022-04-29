cmake_minimum_required(VERSION 3.11)

add_executable(tutorial-simulation_5_solution module_code.cpp)
target_link_libraries(tutorial-simulation_5_solution rofi::hal atoms)
