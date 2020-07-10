cmake_minimum_required(VERSION 3.11)

add_executable(communication communication.cpp)
target_link_libraries(communication rofiHal)

create_world(communication.world communication.in)
create_world(set_1.world set_1.in)
create_world(set_2.world set_2.in)
