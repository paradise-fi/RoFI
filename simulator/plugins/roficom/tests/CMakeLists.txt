cmake_minimum_required(VERSION 3.11)

add_executable(tests_roficom test_main.cpp test_roficom_connect.cpp)
target_link_libraries(tests_roficom Catch2 roficomPlugin)
