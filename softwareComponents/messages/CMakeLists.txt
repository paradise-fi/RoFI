cmake_minimum_required(VERSION 3.11)


add_library(gzMessageServer src/gz/message_server.cpp)
target_link_libraries(gzMessageServer PRIVATE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(gzMessageServer PRIVATE ${GAZEBO_INCLUDE_DIRS})
target_include_directories(gzMessageServer PUBLIC server_include)


add_library(gzMessageClient src/gz/message_client.cpp)
target_link_libraries(gzMessageClient PRIVATE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(gzMessageClient PRIVATE ${GAZEBO_INCLUDE_DIRS})
target_include_directories(gzMessageClient PUBLIC client_include)
