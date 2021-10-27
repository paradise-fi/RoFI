cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-tangle ${ARMADILLO_LIBRARIES} main.cpp)
target_link_libraries(rofi-tangle PUBLIC original-configuration generators dimcli)

