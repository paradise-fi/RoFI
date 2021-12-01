cmake_minimum_required(VERSION 3.11)


set (GZ_SRC_FILES
    gz_message_server.cpp
)

add_library(gzMessageServer ${GZ_SRC_FILES})
target_link_libraries(gzMessageServer PRIVATE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(gzMessageServer PRIVATE ${GAZEBO_INCLUDE_DIRS})
target_include_directories(gzMessageServer PUBLIC include)
