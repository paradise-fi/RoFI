cmake_minimum_required(VERSION 3.6)
project(rofidriver)

set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE SRC src/*.cpp)

add_library(lwip++ INTERFACE)
target_include_directories(lwip++ INTERFACE include)
