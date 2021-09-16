cmake_minimum_required(VERSION 3.11)


set (FILES
    distributor.cpp
    modules_info.cpp
)

add_library(networkingRofiInterface ${FILES})
target_link_libraries(networkingRofiInterface PUBLIC rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(networkingRofiInterface PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(networkingRofiInterface INTERFACE ${CMAKE_CURRENT_LIST_DIR})
