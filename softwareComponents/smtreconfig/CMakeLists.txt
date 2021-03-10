cmake_minimum_required(VERSION 3.6)
project(rofi)

find_package(Z3 4.8 REQUIRED)

add_library(smt-reconfig-lib smtReconfig.cpp smt.cpp smt.hpp)
target_link_libraries(smt-reconfig-lib PUBLIC configuration ${Z3_LIBRARIES})
target_link_libraries(smt-reconfig-lib PRIVATE fmt)
target_include_directories(smt-reconfig-lib INTERFACE .)

add_executable(smt-reconfig main.cpp)
target_link_libraries(smt-reconfig PRIVATE smt-reconfig-lib fmt cxxopts)

add_executable(smt-reconfig-test test/main.cpp test/test.cpp test/smtTest.cpp)
target_link_libraries(smt-reconfig-test PRIVATE fmt smt-reconfig-lib Catch2::Catch2 )
target_include_directories(smt-reconfig-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
catch_discover_tests(smt-reconfig-test)