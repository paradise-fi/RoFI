cmake_minimum_required(VERSION 3.11)

add_executable(communication communication.cpp)
target_link_libraries(communication rofiHal)
