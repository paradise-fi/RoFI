cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SRC src/*.cpp)

add_library(firmwareUpdater ${SRC})
set_property(TARGET firmwareUpdater PROPERTY CXX_STANDARD 17)  # when C++20 is available let's use it (std::span)
set_property(TARGET firmwareUpdater PROPERTY CXX_STANDARD_REQUIRED ON)

add_library(rofi::firmwareUpdater ALIAS firmwareUpdater)
target_link_libraries(firmwareUpdater PRIVATE rofi_hal_inc)
target_include_directories(firmwareUpdater PUBLIC include)
