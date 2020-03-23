cmake_minimum_required(VERSION 3.10)


find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

add_library(roficomPlugin SHARED roficomPlugin.cpp roficomUtils.cpp roficomConnect.cpp)
target_link_libraries(roficomPlugin rofiPluginCommon)
target_include_directories(roficomPlugin INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(tests)
