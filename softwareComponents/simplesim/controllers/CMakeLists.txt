cmake_minimum_required(VERSION 3.11)


set (FILES
    controllers.cpp
)

add_library(networkingControllers ${FILES})
target_link_libraries(networkingControllers PUBLIC networkingRofiInterface networkingSimulation)
target_link_libraries(networkingControllers PUBLIC jthread)
target_include_directories(networkingControllers INTERFACE ${CMAKE_CURRENT_LIST_DIR})
