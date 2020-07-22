cmake_minimum_required(VERSION 3.11)

add_executable(roficomCmd roficomCmd.cpp roficomCmdImpl.cpp)
target_link_libraries(roficomCmd rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
