cmake_minimum_required(VERSION 3.11)


set(FILES
    src/command_handler.cpp
    src/distributor.cpp
    src/locked_module_communication.cpp
    src/module_states.cpp
    src/modules_communication.cpp
    src/simplesim.cpp
)

add_library(simplesim ${FILES})
target_link_libraries(simplesim PUBLIC rofisimMessages configuration simplesimConfigMsgs atoms ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(simplesim SYSTEM PUBLIC ${GAZEBO_INCLUDE_DIRS})
target_include_directories(simplesim PUBLIC include)
