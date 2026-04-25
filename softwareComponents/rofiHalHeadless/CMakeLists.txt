add_library(rofi_hal_headless STATIC rofi_hal.cpp)
target_link_libraries(rofi_hal_headless PUBLIC rofi::hal::inc)

add_library(rofi::hal::headless ALIAS rofi_hal_headless)
