cmake_minimum_required(VERSION 3.10)

add_executable(roficomCommandLineDemo roficom_command_line_demo.cpp roficom_demo_impl.cpp)
target_link_libraries(roficomCommandLineDemo rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
