cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
include(FetchContent)
include(CollectTargets)
include(ESP32)
include(ColoredOutput)

set(ESP32_BUILD 1)

project(rofi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")

# Specify common dependencies
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
  lib_fort
  GIT_REPOSITORY https://github.com/seleznevae/libfort.git
  GIT_TAG        v0.4.2
)
set(FORT_ENABLE_TESTING OFF CACHE BOOL "Enables building tests and examples" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(lib_fort)


function(add_rofi_firmware name)
    cmake_parse_arguments(A "" "FLASH_SIZE" "SOURCES" ${ARGN})
    add_executable(${name} ${A_SOURCES})
    idf_build_get_property(link_options LINK_OPTIONS)
    if(link_options)
        set_property(TARGET ${name} APPEND PROPERTY LINK_OPTIONS "${link_options}")
    endif()

    idf_build_get_property(link_depends __LINK_DEPENDS)
    if(link_depends)
        set_property(TARGET ${name} APPEND PROPERTY LINK_DEPENDS "${link_depends}")
    endif()

    add_dependencies(${name} __idf_build_target build_hack)
    generate_image_from(${name} "${A_FLASH_SIZE}")
endfunction()

function(add_component name)
    add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

function(add_application path)
    add_subdirectory($ENV{ROFI_ROOT}/${path} ${CMAKE_CURRENT_BINARY_DIR}/${path})
endfunction()

idf_build_process(esp32
    COMPONENTS idf::freertos idf::esptool_py idf::esp_http_client idf::esp_netif
               idf::esp_wifi idf::lwip idf::console idf::nvs_flash
               idf::esp_hw_support idf::esp_rom idf::esp_timer idf::app_update
               idf::spi_flash idf::esp_driver_gpio idf::esp_driver_spi
               idf::esp_driver_uart
    SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
    BUILD_DIR ${CMAKE_BINARY_DIR})

# What follows is a huge, huge hack. Unless we link idf::freertos from the
# top-level CMakeLists.txt, we get generator error about self-refereincing
# sources. Therefore, let's create a dummy binary and link it.
set(HACK_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/hack_src.c)
file(GENERATE OUTPUT ${HACK_SOURCE}
    CONTENT "void app_main() {}\n")
add_executable(build_hack ${HACK_SOURCE})
idf_build_executable(build_hack)
target_link_libraries(build_hack idf::freertos)
set_target_properties(build_hack PROPERTIES EXCLUDE_FROM_ALL "exclude-NOTFOUND")

add_component(atoms)
add_component(lwip++)
add_component(lwipcore)
add_component(rofiHalInc)
add_component(rofiHalEsp32)
add_component(libfi)
add_component(firmwareUpdater)
add_component(networking)

# Introduce alias
add_library(rofi::hal ALIAS rofi_hal_esp32)

# Add utilities
add_application(hardwareModules/universalModule/software/esp32/dynamixelConfigurator)
add_application(hardwareModules/universalModule/software/esp32/console)


# Add the examples
add_application(hardwareModules/universalModule/software/esp32/esp_hello_world)
add_application(examples/hal/wifi)
add_application(examples/hal/servo)
add_application(examples/hal/connectorHal)
add_application(examples/hal/connectorMovementHal)
add_application(examples/hal/connectorHalStatus)
add_application(examples/hal/firmwareUpdater)
add_application(examples/hal/communication)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list_targets()
