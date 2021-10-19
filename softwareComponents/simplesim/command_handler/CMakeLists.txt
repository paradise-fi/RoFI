cmake_minimum_required(VERSION 3.11)


set (FILES
    command_handler.cpp
)

add_library(simplesimCommandHandler ${FILES})
target_link_libraries(simplesimCommandHandler PUBLIC simplesimModuleStates rofisimMessages atoms ${Boost_LIBRARIES})
target_include_directories(simplesimCommandHandler INTERFACE ${CMAKE_CURRENT_LIST_DIR})
