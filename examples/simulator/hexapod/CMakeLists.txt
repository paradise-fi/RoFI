cmake_minimum_required(VERSION 3.11)

add_executable(rofi-hexapod hexapod.cpp)
target_link_libraries(rofi-hexapod rofiHal)
