cmake_minimum_required(VERSION 3.11)

add_executable(test-rofiHalStatic test_static_rofi_hal.cpp)
target_link_libraries(test-rofiHalStatic rofi_hal_sim Catch2)
