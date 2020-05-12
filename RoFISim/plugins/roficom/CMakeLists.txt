cmake_minimum_required(VERSION 3.11)

add_library(roficomPlugin SHARED roficomPlugin.cpp roficomUtils.cpp roficomConnect.cpp roficomConnection.cpp)
target_link_libraries(roficomPlugin rofiPluginCommon)
target_include_directories(roficomPlugin INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_library(attacherPlugin SHARED attacherPlugin.cpp)
target_link_libraries(attacherPlugin rofiPluginCommon)

add_subdirectory(tests)
