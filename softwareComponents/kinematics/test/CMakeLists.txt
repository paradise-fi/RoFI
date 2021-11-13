cmake_minimum_required(VERSION 3.11)

add_executable(test_fabrik main.cpp test_fabrik.cpp)
target_link_libraries(test_fabrik Catch2 configuration)
