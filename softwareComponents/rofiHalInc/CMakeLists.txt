add_library(rofi_hal_inc INTERFACE)
target_include_directories(rofi_hal_inc INTERFACE include)
target_link_libraries(rofi_hal_inc INTERFACE lwip++)

add_library(rofi::hal::inc ALIAS rofi_hal_inc)
