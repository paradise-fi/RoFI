cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 17)

add_library(jthread INTERFACE)
target_include_directories(jthread INTERFACE include)
