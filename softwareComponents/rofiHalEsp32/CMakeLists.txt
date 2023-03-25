file(GLOB_RECURSE SRC src/*.cpp src/*.c)

macro(setup_hal NAME)
    add_library("${NAME}" ${SRC})
    target_include_directories("${NAME}" PUBLIC src)
    set_property(TARGET "${NAME}" PROPERTY CXX_STANDARD 20)
    target_link_libraries("${NAME}" PUBLIC idf::driver idf::esp_wifi idf::esp_rom idf::esp_timer atoms rofi::hal::inc)
    target_compile_options("${NAME}" PUBLIC -mtext-section-literals -mlongcalls)
endmacro()

# We provide two versions: the standard, RoFI HAL and...
setup_hal(rofi_hal_esp32)
add_library(rofi::hal::esp32 ALIAS rofi_hal_esp32)

# ...the version that works on RoFI test jig (e.g., it doesn't require the
# presence of servomotors)
setup_hal(rofi_hal_esp32_jig)
target_compile_definitions(rofi_hal_esp32_jig PUBLIC -DROFI_HAL_NO_MOTORS)
add_library(rofi::hal::esp32_jig ALIAS rofi_hal_esp32_jig)
