cmake_minimum_required(VERSION 3.11)

add_executable(distributionMemoryExample main.cpp)
target_link_libraries(distributionMemoryExample rofi::hal networking leadership taskDistribution)