cmake_minimum_required(VERSION 3.11)

add_executable(communication communication.cpp)
target_link_libraries(communication rofi::hal networking)

add_gazebo_world(communication.world communication.in)
add_gazebo_world(set_1.world set_1.in)
add_gazebo_world(set_2.world set_2.in)
add_gazebo_world(set_3.world set_3.in)
add_gazebo_world(set_0.world set_0.in)
