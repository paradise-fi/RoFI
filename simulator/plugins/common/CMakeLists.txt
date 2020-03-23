cmake_minimum_required(VERSION 3.10)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

add_library(rofiPluginCommon INTERFACE)
target_link_libraries(rofiPluginCommon INTERFACE ${GAZEBO_LIBRARIES} rofiMessages)
target_include_directories(rofiPluginCommon INTERFACE ${CMAKE_CURRENT_LIST_DIR})
