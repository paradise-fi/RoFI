cmake_minimum_required(VERSION 3.6)

file(GLOB_RECURSE SRC src/*.cpp)

add_library(taskDistribution ${SRC})
target_link_libraries(taskDistribution PUBLIC leadership networking)
target_include_directories(taskDistribution PUBLIC functions tasks distribution systemMethods memory)
