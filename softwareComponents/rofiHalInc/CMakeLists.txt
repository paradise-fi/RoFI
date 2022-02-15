add_library(rofi_hal_inc INTERFACE)
target_include_directories(rofi_hal_inc PUBLIC INTERFACE include)

add_library(rofi::hal::inc ALIAS rofi_hal_inc)
