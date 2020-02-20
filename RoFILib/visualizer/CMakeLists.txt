cmake_minimum_required(VERSION 3.6)
project(rofi)

add_executable(rofi-vis ${ARMADILLO_LIBRARIES} main.cpp Generator.h Camera.h Animator.h)
target_link_libraries(rofi-vis configuration ${VTK_LIBRARIES})
target_include_directories(rofi-vis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)

add_executable(rofi-vis-test test/main.cpp test/test.cpp)
target_link_libraries(rofi-vis-test configuration ${ARMADILLO_LIBRARIES} ${VTK_LIBRARIES} Catch2::Catch2)
target_include_directories(rofi-vis-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
