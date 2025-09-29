cmake_minimum_required(VERSION 3.11)

add_executable(distributionComplexExample main.cpp)
target_link_libraries(distributionComplexExample rofi::hal networking leadership taskDistribution)