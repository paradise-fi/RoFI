
cmake_minimum_required(VERSION 3.11)


add_library(rofiHal SHARED rofi_hal.cpp)
target_link_libraries(rofiHal PRIVATE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} rofisimMessages atoms)
target_link_libraries(rofiHal PUBLIC lwip++)
target_include_directories(rofiHal SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})
target_include_directories(rofiHal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


add_subdirectory(tests)
