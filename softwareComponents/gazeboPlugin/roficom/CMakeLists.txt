cmake_minimum_required(VERSION 3.11)

add_library(roficomUtils SHARED roficomUtils.cpp roficomConnect.cpp)
target_link_libraries(roficomUtils rofiPluginCommon)
target_include_directories(roficomUtils INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_library(roficomPlugin SHARED roficomPlugin.cpp roficomConnection.cpp)
target_link_libraries(roficomPlugin roficomUtils)

add_library(attacherPlugin SHARED attacherPlugin.cpp)
target_link_libraries(attacherPlugin roficomUtils)

add_subdirectory(tests)
