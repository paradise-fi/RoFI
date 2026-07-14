cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(geometry INTERFACE)
target_include_directories(geometry INTERFACE include)
target_link_libraries(geometry INTERFACE configuration)

if(TARGET Catch2WithMain)
  file(GLOB TEST_SRC test/*.cpp)
  add_executable(test-geometry ${TEST_SRC})
  target_link_libraries(test-geometry PRIVATE Catch2WithMain geometry)
elseif(TARGET Catch2::Catch2)
  message(WARNING "Catch2 available, but not Catch2WithMain. Tests for geometry will not build.")
endif()
