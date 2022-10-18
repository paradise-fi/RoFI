file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_library(rofi_hal_esp32 ${SRC})
set_property(TARGET rofi_hal_esp32 PROPERTY CXX_STANDARD 17)
target_link_libraries(rofi_hal_esp32 PRIVATE esp32++ atoms)
target_link_libraries(rofi_hal_esp32 PUBLIC rofi::hal::inc)
target_compile_options(rofi_hal_esp32 PUBLIC -mtext-section-literals -mlongcalls)

add_library(rofi::hal::esp32 ALIAS rofi_hal_esp32)
