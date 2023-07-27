cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
include(FetchContent)
include(CollectTargets)
include(ESP32)
include(ColoredOutput)

project(rofi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")

# Specify common dependencies
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.7.3
)
FetchContent_GetProperties(magic_enum)
if(NOT magic_enum_POPULATED)
  FetchContent_Populate(magic_enum)
  add_subdirectory(${magic_enum_SOURCE_DIR} ${magic_enum_BINARY_DIR})
endif()

FetchContent_Declare(
  lib_fort
  GIT_REPOSITORY https://github.com/seleznevae/libfort.git
  GIT_TAG        v0.4.2
)
FetchContent_GetProperties(lib_fort)
if(NOT lib_fort_POPULATED)
  FetchContent_Populate(lib_fort)
  option(FORT_ENABLE_TESTING "Enables building tests and examples" OFF)
  add_subdirectory(${lib_fort_SOURCE_DIR} ${lib_fort_BINARY_DIR})
endif()

FetchContent_Declare(
    rbprotocol
    GIT_REPOSITORY https://github.com/RoboticsBrno/RB3201-RBProtocol-library.git
    GIT_TAG        98fee635b3810f404702446c482f1ed13c476740
)
FetchContent_GetProperties(rbprotocol)
if(NOT rbprotocol_POPULATED)
  FetchContent_Populate(rbprotocol)
  file(GLOB_RECURSE SRC ${rbprotocol_SOURCE_DIR}/src/*.cpp ${rbprotocol_SOURCE_DIR}/src/*.c ${rbprotocol_SOURCE_DIR}/src/*.h)
  add_library(rbprotocol ${SRC})
  include_directories(${rbprotocol_SOURCE_DIR}/src)
  target_include_directories(rbprotocol PUBLIC src)
  set_property(TARGET rbprotocol PROPERTY CXX_STANDARD 20)
  target_link_libraries(rbprotocol PUBLIC idf::driver idf::esp_wifi idf::esp_rom idf::esp_timer idf::app_update idf::spiffs idf::nvs_flash atoms rofi::hal::inc)
endif()
add_library(rofi::hal::rbprotocol ALIAS rbprotocol)


FetchContent_Declare(
    gridui
    GIT_REPOSITORY https://github.com/RoboticsBrno/Esp32-RBGridUI.git
    GIT_TAG        ee6ca51f4b7fa2e88d627b00c5fcc19f19a5dcf9
)
FetchContent_GetProperties(gridui)
if(NOT gridui_POPULATED)
  FetchContent_Populate(gridui)

  file(GLOB_RECURSE SRC ${gridui_SOURCE_DIR}/src/*.cpp ${gridui_SOURCE_DIR}/src/*.c ${gridui_SOURCE_DIR}/src/*.h)
  add_library(gridui ${SRC})
  include_directories(${gridui_SOURCE_DIR}/src)
  target_include_directories(gridui PUBLIC src)
  set_property(TARGET gridui PROPERTY CXX_STANDARD 20)
  target_link_libraries(gridui PUBLIC idf::driver idf::esp_wifi idf::esp_rom idf::esp_timer idf::app_update idf::spiffs idf::nvs_flash atoms rofi::hal::inc rofi::hal::rbprotocol)
endif()
add_library(rofi::hal::gridui ALIAS gridui)


function(add_rofi_firmware name)
    cmake_parse_arguments(A "" "FLASH_SIZE" "SOURCES" ${ARGN})
    add_executable(${name} ${A_SOURCES})
    idf_build_executable(${name})
    generate_image_from(${name} "${A_FLASH_SIZE}")
endfunction()

function(add_component name)
    add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

function(add_application path)
    add_subdirectory($ENV{ROFI_ROOT}/${path} ${CMAKE_CURRENT_BINARY_DIR}/${path})
endfunction()

idf_build_process(esp32
    COMPONENTS idf::freertos idf::esptool_py idf::esp_http_client idf::esp_wifi
               idf::lwip idf::console idf::nvs_flash idf::esp_hw_support idf::esp_rom idf::spiffs
               idf::esp_timer idf::vfs idf::spi_flash idf::esp_eth idf::esp_netif idf::newlib idf::fatfs idf::pthread
    SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
    BUILD_DIR ${CMAKE_BINARY_DIR})

FetchContent_Declare(
    Jaculus-dcore
    GIT_REPOSITORY https://github.com/cubicap/Jaculus-dcore.git
    GIT_TAG        v0.0.10
)

FetchContent_GetProperties(Jaculus-dcore)
if(NOT Jaculus-dcore_POPULATED)
  FetchContent_Populate(jaculus-dcore)
  add_subdirectory(${jaculus-dcore_SOURCE_DIR} ${jaculus-dcore_BINARY_DIR})

  # Jaculus doesn't link to idf::newlib (which IDF component does automatically, let's fix it
  target_link_libraries(jac-machine PUBLIC idf::newlib)
  target_link_libraries(jac-link INTERFACE idf::newlib)
  target_link_libraries(quickjs PUBLIC idf::newlib -DESP_PLATFORM)
  target_compile_options(quickjs PUBLIC -mtext-section-literals -mlongcalls)
endif()






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
add_component(rofiHalJaculus)
# Introduce alias
add_library(rofi::hal ALIAS rofi_hal_esp32)

# Add utilities
add_application(hardwareModules/universalModule/software/esp32/dynamixelConfigurator)
add_application(hardwareModules/universalModule/software/esp32/console)
add_application(hardwareModules/universalModule/software/esp32/jaculus)
add_application(hardwareModules/universalModule/software/esp32/gridui)


# Add the examples
add_application(hardwareModules/universalModule/software/esp32/esp_hello_world)
add_application(examples/hal/wifi)
add_application(examples/hal/servo)
add_application(examples/hal/connectorHal)
add_application(examples/hal/connectorMovementHal)
add_application(examples/hal/connectorHalStatus)
add_application(examples/hal/firmwareUpdater)



set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list_targets()
