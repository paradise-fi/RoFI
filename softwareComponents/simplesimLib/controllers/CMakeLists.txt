cmake_minimum_required(VERSION 3.11)


set (FILES
    controllers.cpp
)

add_library(simplesimControllers ${FILES})
target_link_libraries(simplesimControllers PUBLIC simplesimCommunication simplesimSimulation)
target_include_directories(simplesimControllers INTERFACE ${CMAKE_CURRENT_LIST_DIR})
