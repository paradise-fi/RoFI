cmake_minimum_required(VERSION 3.6)

add_library(leadership)
target_link_libraries(leadership PUBLIC networking pthread)
target_include_directories(leadership PUBLIC include)
