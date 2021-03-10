cmake_minimum_required(VERSION 3.12)

project(rofi_doc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)

include(CollectTargets)

set(DOC_PATH "${CMAKE_CURRENT_BINARY_DIR}/web")

add_subdirectory($ENV{ROFI_ROOT}/doc doc)


list_targets()
