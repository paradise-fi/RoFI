cmake_minimum_required(VERSION 3.11)


add_executable(rofi-simplesimClient main.cpp)
target_link_libraries(rofi-simplesimClient simplesimClient messageServer configurationWithJson)
