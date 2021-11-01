cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-generate ${ARMADILLO_LIBRARIES} main.cpp)
target_link_libraries(rofi-generate PUBLIC legacy-configuration generators)

