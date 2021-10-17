cmake_minimum_required(VERSION 3.11)


set (FILES
    distributor.cpp
    modules_info.cpp
)

add_library(simplesimCommunication ${FILES})
target_link_libraries(simplesimCommunication PUBLIC rofisimMessages atoms ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(simplesimCommunication PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(simplesimCommunication INTERFACE ${CMAKE_CURRENT_LIST_DIR})
