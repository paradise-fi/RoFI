cmake_minimum_required(VERSION 3.10)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS} ${SOURCE_CODE_FLAGS}")

add_executable(basic_joint_velocity basic_joint_velocity.cpp)
target_link_libraries(basic_joint_velocity rofiHal)
