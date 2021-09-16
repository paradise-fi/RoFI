cmake_minimum_required(VERSION 3.11)


set (FILES
    gazebo_master.cpp
)

add_library(networkingGazeboMaster ${FILES})
target_link_libraries(networkingGazeboMaster PUBLIC ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(networkingGazeboMaster PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(networkingGazeboMaster INTERFACE ${CMAKE_CURRENT_LIST_DIR})
