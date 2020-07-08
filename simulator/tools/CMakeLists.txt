cmake_minimum_required(VERSION 3.11)

add_executable(attacher attacher.cpp)
target_link_libraries(attacher rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(listener listener.cpp)
target_link_libraries(listener rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})

add_executable(ping ping.cpp)
target_link_libraries(ping rofiHal)


add_subdirectory(worldCreator)
