cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
include(FetchContent)
include(CollectTargets)
include(ESP32)
include(ColoredOutput)

project(rofi)

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
    COMPONENTS idf::esp32 idf::freertos idf::esptool_py idf::esp_http_client
               idf::lwip idf::console
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
# Introduce alias
add_library(rofi::hal ALIAS rofi_hal_esp32)

# Add utilities
add_application(hardwareModules/universalModule/software/esp32/dynamixelConfigurator)

# Add the examples
add_application(hardwareModules/universalModule/software/esp32/esp_hello_world)
add_application(examples/hal/wifi)
add_application(examples/hal/servo)
add_application(examples/hal/connectorMovementHal)



set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list_targets()
