
cmake_minimum_required(VERSION 3.11)


add_library(rofi_hal_sim SHARED rofi_hal.cpp)
target_link_libraries(rofi_hal_sim PRIVATE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} rofisimMessages atoms)
target_link_libraries(rofi_hal_sim PUBLIC lwip++ rofi::hal::inc)
target_include_directories(rofi_hal_sim SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})

add_library(rofi::hal::sim ALIAS rofi_hal_sim)

add_subdirectory(tests)
