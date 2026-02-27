cmake_minimum_required(VERSION 3.11)

add_executable(distributionSimpleExample main.cpp)
target_link_libraries(distributionSimpleExample rofi::hal networking leadership taskDistribution)