cmake_minimum_required(VERSION 3.6)
project(rofi)

find_package(Armadillo REQUIRED)

FetchContent_Declare(
  span
  GIT_REPOSITORY https://github.com/tcbrindle/span
)
FetchContent_GetProperties(catch)
if(NOT span_POPULATED)
  FetchContent_Populate(span)
  add_subdirectory(${span_SOURCE_DIR} ${span_BINARY_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${span_SOURCE_DIR}/contrib")
endif()

file(GLOB LEGACY_CONF_SRC *)
add_library(legacy-configuration STATIC ${LEGACY_CONF_SRC})
target_include_directories(legacy-configuration PUBLIC .)
target_link_libraries(legacy-configuration PUBLIC ${ARMADILLO_LIBRARIES})


file(GLOB CONFIGURATION_SRC src/*)
add_library(configuration STATIC ${CONFIGURATION_SRC})
target_include_directories(configuration PUBLIC include)
target_link_libraries(configuration PUBLIC ${ARMADILLO_LIBRARIES} atoms span fmt)

add_library(generators STATIC Generators.cpp Generators.h)
target_include_directories(generators PUBLIC .)
target_link_libraries(generators PUBLIC legacy-configuration)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-configuration ${TEST_SRC})
target_link_libraries(test-configuration PRIVATE Catch2::Catch2 configuration atoms)
