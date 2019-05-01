cmake_minimum_required(VERSION 3.6)

find_package(Wget REQUIRED)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

ExternalProject_Add(
  catch2-project
  PREFIX ${CMAKE_BINARY_DIR}/catch2
  DOWNLOAD_DIR catch2
  DOWNLOAD_COMMAND ${WGET_EXECUTABLE} https://github.com/catchorg/Catch2/releases/download/v2.7.2/catch.hpp
  TIMEOUT 10
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(catch2-project download_dir)

add_library(catch2 INTERFACE)
add_dependencies(catch2 catch2-project)
target_include_directories(catch2 INTERFACE ${download_dir})
add_library(catch2::catch ALIAS catch2)

project(control_board_logic_test CXX)

file(GLOB_RECURSE SRC src/*.cpp)
set(FW_SRC ${CMAKE_SOURCE_DIR}/../../src)

add_executable(test ${SRC})

target_include_directories(test PRIVATE ${FW_SRC})
target_link_libraries(test catch2::catch)
set_property(TARGET test PROPERTY CXX_STANDARD 17)
