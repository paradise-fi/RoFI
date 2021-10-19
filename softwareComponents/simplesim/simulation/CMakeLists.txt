cmake_minimum_required(VERSION 3.11)


set (FILES
    simulation.cpp
)

add_library(simplesimSimulation ${FILES})
target_link_libraries(simplesimSimulation PUBLIC simplesimModuleStates simplesimCommandHandler rofisimMessages)
target_include_directories(simplesimSimulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
