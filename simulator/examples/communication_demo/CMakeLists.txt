cmake_minimum_required(VERSION 3.11)

add_executable(communicationDemo communication_demo.cpp)
target_link_libraries(communicationDemo rofiHal)
