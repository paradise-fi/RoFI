
cmake_minimum_required(VERSION 3.11)

add_library(rofiHal SHARED rofi_hal.cpp)
target_link_libraries(rofiHal PUBLIC rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofiHal INTERFACE ${CMAKE_CURRENT_LIST_DIR})


add_subdirectory(tests)
