cmake_minimum_required(VERSION 3.11)

add_executable(commandLineDemo command_line_demo.cpp)
target_link_libraries(commandLineDemo rofiHal)
