cmake_minimum_required(VERSION 3.11)

add_library(roficomUtils SHARED roficomUtils.cpp roficomConnect.cpp)
target_link_libraries(roficomUtils rofiPluginCommon)
target_include_directories(roficomUtils PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(roficomUtils INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_library(roficomPlugin SHARED roficomPlugin.cpp roficomConnection.cpp)
target_link_libraries(roficomPlugin roficomUtils)
target_include_directories(roficomPlugin PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_library(attacherPlugin SHARED attacherPlugin.cpp)
target_link_libraries(attacherPlugin roficomUtils)
target_include_directories(attacherPlugin PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_subdirectory(tests)
