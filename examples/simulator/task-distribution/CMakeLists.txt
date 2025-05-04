cmake_minimum_required(VERSION 3.11)

add_executable(distributionConcept main.cpp)
target_link_libraries(distributionConcept rofi::hal networking leadership taskDistribution)