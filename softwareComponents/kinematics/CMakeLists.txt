cmake_minimum_required(VERSION 3.6)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

add_library(kinematics fabrik.cpp calculations.hpp)
target_include_directories(kinematics INTERFACE .)
target_link_libraries(kinematics PRIVATE configuration)

add_executable(test-rofi-ik ${modelResources} test/main.cpp test/test_fabrik.cpp )
target_link_libraries(test-rofi-ik PUBLIC configuration atoms atoms-heavy ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES} Catch2::Catch2 kinematics)
target_include_directories(test-rofi-ik PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
