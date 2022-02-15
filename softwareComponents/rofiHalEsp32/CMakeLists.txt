file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_library(rofi_hal_esp32 ${SRC})
target_include_directories(rofi_hal_esp32 PUBLIC src)
set_property(TARGET rofi_hal_esp32 PROPERTY CXX_STANDARD 17)
target_link_libraries(rofi_hal_esp32 PUBLIC idf::driver atoms lwip++ rofi_hal_inc)
target_compile_options(rofi_hal_esp32 PUBLIC -mtext-section-literals -mlongcalls)

add_library(rofi::hal::esp32 ALIAS rofi_hal_esp32)
