cmake_minimum_required(VERSION 3.10)

add_executable(packetDemo packet_demo.cpp)
target_link_libraries(packetDemo rofiHal)
