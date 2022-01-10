cmake_minimum_required(VERSION 3.11)

add_executable(failing_test-gazebosimRoficom test_main.cpp test_roficom_connect.cpp)
target_link_libraries(failing_test-gazebosimRoficom Catch2 roficomPlugin)
