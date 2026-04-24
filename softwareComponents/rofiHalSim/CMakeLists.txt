
cmake_minimum_required(VERSION 3.11)

set(SRC
    rofi_hal.cpp
    session_id.cpp
)

add_library(rofi_hal_sim SHARED ${SRC})
target_link_libraries(rofi_hal_sim PRIVATE gzTransportCompat rofisimMessages atoms)
target_link_libraries(rofi_hal_sim PUBLIC rofi::hal::inc)

add_library(rofi::hal::sim ALIAS rofi_hal_sim)

add_subdirectory(tests)
