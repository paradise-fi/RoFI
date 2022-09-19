file(GLOB_RECURSE SRC src/*.cpp src/*.c)
list(REMOVE_ITEM SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/rofi_hal.cpp
                     ${CMAKE_CURRENT_SOURCE_DIR}/src/rofi_hal_serial.cpp)

if(${USE_DUMMY_HAL})
    list(APPEND SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/rofi_hal_serial.cpp)
else()
    list(APPEND SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/rofi_hal.cpp)
endif()

add_library(rofi_hal_esp32 ${SRC})
target_include_directories(rofi_hal_esp32 PUBLIC src)
set_property(TARGET rofi_hal_esp32 PROPERTY CXX_STANDARD 17)
target_link_libraries(rofi_hal_esp32 PUBLIC idf::driver atoms lwip++ rofi_hal_inc)
target_compile_options(rofi_hal_esp32 PUBLIC -mtext-section-literals -mlongcalls)

add_library(rofi::hal::esp32 ALIAS rofi_hal_esp32)
