cmake_minimum_required(VERSION 3.13)
project(rofi)

find_package(Z3)

add_library(smt-reconfig-lib smtReconfig.cpp)
target_link_libraries(smt-reconfig-lib PUBLIC configuration ${Z3_LIBRARIES})
target_link_libraries(smt-reconfig-lib PRIVATE fmt)

add_executable(smt-reconfig main.cpp)
target_link_libraries(smt-reconfig PRIVATE smt-reconfig-lib)

add_executable(smt-reconfig-test test/main.cpp test/test.cpp)
target_link_libraries(smt-reconfig-test smt-reconfig-lib)
target_include_directories(smt-reconfig-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
