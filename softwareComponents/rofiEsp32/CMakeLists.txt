file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_library(rofi_esp32 STATIC ${SRC})
set_property(TARGET rofi_esp32 PROPERTY CXX_STANDARD 17)
target_compile_options(rofi_esp32 PUBLIC -mtext-section-literals -mlongcalls)
target_include_directories(rofi_esp32 PUBLIC src)
target_link_libraries(rofi_esp32 PUBLIC idf::driver idf::esp_wifi idf::nvs_flash idf::esp_http_client)

add_library(rofi::esp32 ALIAS rofi_esp32)
