file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_library(libfi STATIC ${SRC})
set_property(TARGET libfi PROPERTY CXX_STANDARD 17)
target_compile_options(libfi PUBLIC -mtext-section-literals -mlongcalls)
target_include_directories(libfi PUBLIC src)
target_link_libraries(libfi PUBLIC idf::driver idf::esp_wifi idf::nvs_flash idf::esp_http_client)
