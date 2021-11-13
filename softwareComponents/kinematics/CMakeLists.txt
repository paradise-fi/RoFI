cmake_minimum_required(VERSION 3.6)
project(rofi)


add_library(kinematics kinematics.cpp calculations.hpp)
target_include_directories(kinematics PUBLIC .)
target_link_libraries(kinematics PRIVATE legacy-configuration)

add_executable(test-rofi-ik ${modelResources} test/main.cpp test/test_fabrik.cpp )
target_link_libraries(test-rofi-ik PUBLIC legacy-configuration atoms atoms-heavy ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES} Catch2::Catch2 kinematics)
target_include_directories(test-rofi-ik PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
