cmake_minimum_required(VERSION 3.11)


add_executable(rofi-gazebosim-attacher attacher.cpp)
target_link_libraries(rofi-gazebosim-attacher rofisimMessages gzTransportCompat ${Boost_LIBRARIES})

add_executable(rofi-sim-listener listener.cpp)
target_link_libraries(rofi-sim-listener rofisimMessages gzTransportCompat ${Boost_LIBRARIES})

add_executable(rofi-sim-ping ping.cpp)
target_link_libraries(rofi-sim-ping rofi::hal)

add_executable(rofi-sim-commandLine commandLine.cpp)
target_link_libraries(rofi-sim-commandLine PRIVATE rofi::hal)

add_executable(rofi-sim-roficomCmd roficomCmd.cpp)
target_link_libraries(rofi-sim-roficomCmd PRIVATE rofisimMessages gzTransportCompat ${Boost_LIBRARIES})


add_subdirectory(worldCreator)
