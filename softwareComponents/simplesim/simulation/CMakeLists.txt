cmake_minimum_required(VERSION 3.11)


set (FILES
    module_states.cpp
    simulation.cpp
)

add_library(simplesimSimulation ${FILES})
target_link_libraries(simplesimSimulation PUBLIC rofisimMessages configuration ${Boost_LIBRARIES})
target_include_directories(simplesimSimulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
