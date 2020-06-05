
cmake_minimum_required(VERSION 3.11)

add_library(rofiHal SHARED rofi_hal.cpp)
target_link_libraries(rofiHal PUBLIC rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofiHal INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../RoFIDriver/atoms ${CMAKE_CURRENT_BINARY_DIR}/atoms)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../RoFIDriver/lwip++ ${CMAKE_CURRENT_BINARY_DIR}/lwip++)

target_link_libraries(rofiHal PUBLIC atoms lwip++)


add_subdirectory(tests)
