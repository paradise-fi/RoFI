cmake_minimum_required(VERSION 3.12)

project(rofi_doc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)

include(CollectTargets)
include(Kikit)

add_subdirectory($ENV{ROFI_ROOT}/hardwareModules/universalModule/hardware universalModule)
add_subdirectory($ENV{ROFI_ROOT}/hardwareModules/RoFICoM/hardware roficom)


list_targets()
