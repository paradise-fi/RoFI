cmake_minimum_required(VERSION 3.11)

add_library(distributorPlugin SHARED distributorPlugin.cpp)
target_link_libraries(distributorPlugin rofiPluginCommon)
target_include_directories(distributorPlugin SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})
