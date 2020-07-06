cmake_minimum_required(VERSION 3.11)

add_executable(ping ping.cpp)
target_link_libraries(ping rofiHal)
