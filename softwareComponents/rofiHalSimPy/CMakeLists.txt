cmake_minimum_required(VERSION 3.11)


pybind11_add_module(pyRofiHal hal.cpp)
target_link_libraries(pyRofiHal PRIVATE rofi::hal)
