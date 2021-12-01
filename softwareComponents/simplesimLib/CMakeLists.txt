cmake_minimum_required(VERSION 3.11)


add_subdirectory(module_states)
add_subdirectory(simulation)
add_subdirectory(command_handler)
add_subdirectory(communication)
add_subdirectory(controllers)


add_library(simplesim ALIAS simplesimControllers)
