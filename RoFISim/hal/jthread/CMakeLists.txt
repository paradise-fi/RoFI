cmake_minimum_required(VERSION 3.11)

add_library(jthread INTERFACE)
target_include_directories(jthread INTERFACE ${CMAKE_CURRENT_LIST_DIR})
