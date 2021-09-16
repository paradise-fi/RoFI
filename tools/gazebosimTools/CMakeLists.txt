cmake_minimum_required(VERSION 3.11)


add_executable(attacher attacher.cpp)
target_link_libraries(attacher rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(attacher PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(listener listener.cpp)
target_link_libraries(listener rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(listener PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(ping ping.cpp)
target_link_libraries(ping rofiHal)

add_executable(commandLine commandLine.cpp)
target_link_libraries(commandLine rofiHal)
target_include_directories(commandLine PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_executable(roficomCmd roficomCmd.cpp)
target_link_libraries(roficomCmd rofisimMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(roficomCmd PRIVATE ${GAZEBO_INCLUDE_DIRS})


add_subdirectory(worldCreator)
