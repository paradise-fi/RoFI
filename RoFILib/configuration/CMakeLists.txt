cmake_minimum_required(VERSION 3.13)
project(rofi)

add_library(configuration INTERFACE)
target_include_directories(configuration INTERFACE ${ARMADILLO_INCLUDE_DIRS})
target_include_directories(configuration INTERFACE .)
target_link_libraries(configuration INTERFACE ${ARMADILLO_LIBRARIES})
