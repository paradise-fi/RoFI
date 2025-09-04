cmake_minimum_required(VERSION 3.11)

add_executable(distributionBarrierExample main.cpp)
target_link_libraries(distributionBarrierExample rofi::hal networking leadership taskDistribution)