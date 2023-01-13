cmake_minimum_required(VERSION 3.11)


add_library(simplesimServer INTERFACE)
target_link_libraries(simplesimServer INTERFACE atoms configuration parsing-lite simplesimPyFilter dimcli)
target_include_directories(simplesimServer INTERFACE include)
