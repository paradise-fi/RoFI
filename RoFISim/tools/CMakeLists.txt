cmake_minimum_required(VERSION 3.11)

add_executable(listener listener.cpp)
target_link_libraries(listener rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
