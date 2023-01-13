cmake_minimum_required(VERSION 3.11)


add_executable(rofi-convert main.cpp)
target_link_libraries(rofi-convert PRIVATE dimcli atoms parsing configuration)
