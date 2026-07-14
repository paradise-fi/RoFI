cmake_minimum_required(VERSION 3.11)

add_executable(test-aabb aabb.cpp)
target_link_libraries(test-aabb Catch2WithMain aabb)
