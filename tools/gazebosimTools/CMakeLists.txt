cmake_minimum_required(VERSION 3.11)


add_executable(attacher attacher.cpp)
target_link_libraries(attacher rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(listener listener.cpp)
target_link_libraries(listener rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(ping ping.cpp)
target_link_libraries(ping rofiHal)

add_executable(commandLine commandLine.cpp)
target_link_libraries(commandLine rofiHal)

add_executable(roficomCmd roficomCmd.cpp)
target_link_libraries(roficomCmd rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})


add_subdirectory(worldCreator)
