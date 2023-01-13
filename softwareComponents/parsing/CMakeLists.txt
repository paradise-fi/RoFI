cmake_minimum_required(VERSION 3.11)


add_library(parsing-lite INTERFACE)
target_link_libraries(parsing-lite INTERFACE atoms configurationWithJson)
target_include_directories(parsing-lite INTERFACE include_lite)

add_library(parsing INTERFACE)
target_link_libraries(parsing INTERFACE parsing-lite voxel)
target_include_directories(parsing INTERFACE include)
