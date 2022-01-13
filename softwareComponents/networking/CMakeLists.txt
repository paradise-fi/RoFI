cmake_minimum_required(VERSION 3.6)
project(rofidriver)

file(GLOB_RECURSE SRC src/*.cpp)

add_library(networking INTERFACE)
target_link_libraries(networking INTERFACE atoms)
target_include_directories(networking INTERFACE include)
