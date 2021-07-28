cmake_minimum_required(VERSION 3.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)


include(FetchContent)
include(CollectTargets)
include(STM32Device)
set(CMAKE_TOOLCHAIN_FILE STM32Toolchain)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(firmware)

add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/stm32cxx stm32cxx)
add_subdirectory($ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board/
roficom)

list_targets()
