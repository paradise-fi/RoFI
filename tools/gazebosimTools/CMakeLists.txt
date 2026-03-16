cmake_minimum_required(VERSION 3.11)


add_executable(rofi-gazebosim-attacher attacher.cpp)
target_link_libraries(rofi-gazebosim-attacher rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofi-gazebosim-attacher SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(rofi-sim-listener listener.cpp)
target_link_libraries(rofi-sim-listener rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofi-sim-listener SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(rofi-sim-ping ping.cpp)
target_link_libraries(rofi-sim-ping rofi::hal)

add_executable(rofi-sim-commandLine commandLine.cpp)
target_link_libraries(rofi-sim-commandLine PRIVATE rofi::hal ${GAZEBO_LIBRARIES} atoms)
target_include_directories(rofi-sim-commandLine SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(rofi-sim-roficomCmd roficomCmd.cpp)
target_link_libraries(rofi-sim-roficomCmd PRIVATE rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofi-sim-roficomCmd SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})


add_subdirectory(worldCreator)
