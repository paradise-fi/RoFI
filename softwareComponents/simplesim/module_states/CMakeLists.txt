cmake_minimum_required(VERSION 3.11)


set (FILES
    module_states.cpp
)

add_library(simplesimModuleStates ${FILES})
target_link_libraries(simplesimModuleStates PUBLIC rofisimMessages configuration)
target_include_directories(simplesimModuleStates INTERFACE ${CMAKE_CURRENT_LIST_DIR})
