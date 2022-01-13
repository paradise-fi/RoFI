cmake_minimum_required(VERSION 3.11)

add_library(rofiModulePlugin SHARED rofiModulePlugin.cpp)
target_link_libraries(rofiModulePlugin rofiPluginCommon)
target_include_directories(rofiModulePlugin SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})
