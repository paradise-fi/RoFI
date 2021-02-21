cmake_minimum_required(VERSION 3.11)


set (FILES
    simulation.cpp
)

add_library(networkingSimulation ${FILES})
target_link_libraries(networkingSimulation PUBLIC rofiMessages ${Boost_LIBRARIES})
target_include_directories(networkingSimulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
