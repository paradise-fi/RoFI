cmake_minimum_required(VERSION 3.6)
project(rofi)

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

add_library(configuration STATIC Configuration.cpp joints.cpp
  universalModule.cpp rofibot.cpp)
target_include_directories(configuration PUBLIC .)
target_link_libraries(configuration PUBLIC ${ARMADILLO_LIBRARIES} PUBLIC atoms span fmt)

add_library(generators STATIC Generators.cpp Generators.h)
target_include_directories(generators PUBLIC .)
target_link_libraries(generators PUBLIC configuration)

file(GLOB TEST_SRC test/*.cpp)
add_executable(configuration-test ${TEST_SRC})
target_link_libraries(configuration-test PRIVATE Catch2::Catch2 configuration atoms)
target_include_directories(configuration-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
catch_discover_tests(configuration-test)
