cmake_minimum_required(VERSION 3.6)

set(EXTRA_COMPONENT_DIRS "$ENV{ROFI_ROOT}/softwareComponents/rofiHalPhys")

set(COMPONENTS "")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(servo)
