cmake_minimum_required(VERSION 3.11)


add_subdirectory(module_states)
add_subdirectory(simulation)
add_subdirectory(command_handler)
add_subdirectory(communication)
add_subdirectory(controllers)

set (SRC
    main.cpp
)

add_executable(rofi-simplesim ${SRC})
target_link_libraries(rofi-simplesim PUBLIC gzMessageServer)
target_link_libraries(rofi-simplesim PUBLIC simplesimControllers)
target_link_libraries(rofi-simplesim PUBLIC configuration dimcli)
