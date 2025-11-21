cmake_minimum_required(VERSION 3.11)

add_executable(distributionMemoryTest main.cpp)
target_link_libraries(distributionMemoryTest rofi::hal networking leadership taskDistribution)