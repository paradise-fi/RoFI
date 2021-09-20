cmake_minimum_required(VERSION 3.11)


set (FILES
    distributor.cpp
    modules_info.cpp
)

add_library(simplesimRofiInterface ${FILES})
target_link_libraries(simplesimRofiInterface PUBLIC rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(simplesimRofiInterface PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(simplesimRofiInterface INTERFACE ${CMAKE_CURRENT_LIST_DIR})
