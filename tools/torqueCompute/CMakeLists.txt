cmake_minimum_required(VERSION 3.11)



add_executable(rofi-torqueCompute main.cpp)
target_link_libraries(rofi-torqueCompute PRIVATE torqueComputation parsing configurationWithJson)