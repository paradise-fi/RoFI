cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
include(FetchContent)
include(CollectTargets)
include(ESP32)
include(ColoredOutput)

project(rofi)

function(add_rofi_firmware name)
    add_executable(${name} ${ARGN})
    idf_build_executable(${name})
    generate_image_from(${name})
endfunction()

function(add_component name)
    add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

function(add_application path)
    add_subdirectory($ENV{ROFI_ROOT}/${path} ${CMAKE_CURRENT_BINARY_DIR}/${path})
endfunction()

idf_build_process(esp32
    COMPONENTS idf::esp32 idf::freertos idf::esptool_py idf::esp_http_client idf::lwip
    SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
    BUILD_DIR ${CMAKE_BINARY_DIR})

# What follows is a huge, huge hack. Unless we link idf::freertos from the
# top-level CMakeLists.txt, we get generator error about self-refereincing
# sources. Therefore, let's create a dummy binary and link it.
set(HACK_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/hack_src.c)
file(GENERATE OUTPUT ${HACK_SOURCE}
    CONTENT "void app_main() {}\n")
add_executable(build_hack ${HACK_SOURCE})
target_link_libraries(build_hack idf::freertos)
set_target_properties(build_hack PROPERTIES EXCLUDE_FROM_ALL "exclude-NOTFOUND")

add_component(atoms)
add_component(lwip++)
add_component(lwipcore)
add_component(rofiHalInc)
add_component(rofiHalEsp32)
add_component(libfi)
# Introduce alias
add_library(rofi::hal ALIAS rofi_hal_esp32)

# Add the examples
add_application(hardwareModules/universalModule/software/esp32/esp_hello_world)
add_application(examples/hal/wifi)


set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list_targets()
