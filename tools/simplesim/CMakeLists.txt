cmake_minimum_required(VERSION 3.10)


add_executable(rofi-simplesim main.cpp)
target_link_libraries(rofi-simplesim simplesim simplesimServer simplesimClient messageServer configuration simplesimPyFilter dimcli)
