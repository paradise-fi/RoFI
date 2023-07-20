file(GLOB_RECURSE SRC src/*.cpp src/*.c)

macro(setup_hal NAME)
    add_library("${NAME}" ${SRC})
    target_include_directories("${NAME}" PUBLIC src)
    set_property(TARGET "${NAME}" PROPERTY CXX_STANDARD 20)
    target_link_libraries("${NAME}" PUBLIC idf::driver idf::esp_wifi idf::esp_rom idf::esp_timer idf::app_update idf::spiffs idf::nvs_flash atoms rofi::hal::inc)
    target_compile_options("${NAME}" PUBLIC -mtext-section-literals -mlongcalls)
endmacro()

# We provide two versions: the standard, RoFI HAL and...
setup_hal(rofiHalGridUi)
add_library(rofi::hal::gridui ALIAS rofiHalGridUi)