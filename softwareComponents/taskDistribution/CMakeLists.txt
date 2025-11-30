cmake_minimum_required(VERSION 3.6)

add_library(taskDistribution)
target_link_libraries(taskDistribution PUBLIC leadership networking)
target_include_directories(taskDistribution PUBLIC functions tasks distribution systemMethods memory)
