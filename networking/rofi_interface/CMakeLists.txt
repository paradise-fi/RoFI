cmake_minimum_required(VERSION 3.11)


set (FILES
    distributor.cpp
    modules_info.cpp
)

add_library(rofiInterface SHARED ${FILES})
target_link_libraries(rofiInterface PUBLIC rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofiInterface INTERFACE ${CMAKE_CURRENT_LIST_DIR})
