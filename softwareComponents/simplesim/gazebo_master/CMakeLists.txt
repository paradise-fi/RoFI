cmake_minimum_required(VERSION 3.11)


set (FILES
    gazebo_master.cpp
)

add_library(simplesimGazeboMaster ${FILES})
target_link_libraries(simplesimGazeboMaster PUBLIC ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(simplesimGazeboMaster PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(simplesimGazeboMaster INTERFACE ${CMAKE_CURRENT_LIST_DIR})
