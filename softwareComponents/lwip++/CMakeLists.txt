cmake_minimum_required(VERSION 3.6)
project(rofidriver)

file(GLOB_RECURSE SRC src/*.cpp networking.cpp)

add_library(lwip++ INTERFACE)
target_link_libraries(lwip++ INTERFACE lwipcore)
target_include_directories(lwip++ INTERFACE include)
target_link_libraries(lwip++ INTERFACE lwipcore)
