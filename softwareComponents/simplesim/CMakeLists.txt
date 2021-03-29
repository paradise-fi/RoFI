cmake_minimum_required(VERSION 3.11)


add_subdirectory(gazebo_master)
add_subdirectory(rofi_interface)
add_subdirectory(simulation)
add_subdirectory(controllers)

set (SRC
    main.cpp
)

add_executable(master ${SRC})
target_link_libraries(master PUBLIC networkingGazeboMaster)
target_link_libraries(master PUBLIC networkingRofiInterface)
target_link_libraries(master PUBLIC networkingSimulation)
target_link_libraries(master PUBLIC networkingControllers)
