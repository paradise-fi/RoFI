cmake_minimum_required(VERSION 3.11)

add_executable(networkManager networkManager.cpp)
target_link_libraries(networkManager rofi::hal networking)
