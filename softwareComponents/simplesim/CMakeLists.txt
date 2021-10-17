cmake_minimum_required(VERSION 3.11)


add_subdirectory(gazebo_master)
add_subdirectory(communication)
add_subdirectory(simulation)
add_subdirectory(controllers)

set (SRC
    main.cpp
)

add_executable(rofi-simplesim ${SRC})
target_link_libraries(rofi-simplesim PUBLIC simplesimGazeboMaster)
target_link_libraries(rofi-simplesim PUBLIC simplesimCommunication)
target_link_libraries(rofi-simplesim PUBLIC simplesimSimulation)
target_link_libraries(rofi-simplesim PUBLIC simplesimControllers)
target_link_libraries(rofi-simplesim PUBLIC configuration dimcli)
