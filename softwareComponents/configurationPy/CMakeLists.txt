cmake_minimum_required(VERSION 3.11)


pybind11_add_module(pyConfiguration configuration.cpp)

target_link_libraries(pyConfiguration PRIVATE armadillo carma)
target_link_libraries(pyConfiguration PRIVATE rofi::configuration)
