cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

set (COMMON_SRC
        utils.cpp
    )

add_library(rofiPluginCommon SHARED ${COMMON_SRC})
target_link_libraries(rofiPluginCommon ${GAZEBO_LIBRARIES} rofiMessages)
target_include_directories(rofiPluginCommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
