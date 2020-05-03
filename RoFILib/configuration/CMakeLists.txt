cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(configuration STATIC Configuration.cpp Configuration.h joints.cpp)
target_include_directories(configuration PUBLIC .)
target_link_libraries(configuration PUBLIC ${ARMADILLO_LIBRARIES} PUBLIC atoms)

add_library(generators STATIC Generators.cpp Generators.h)
target_include_directories(generators PUBLIC .)
target_link_libraries(generators PUBLIC configuration)

file(GLOB TEST_SRC test/*.cpp)
add_executable(configuration-test ${TEST_SRC})
target_link_libraries(configuration-test PRIVATE Catch2::Catch2 configuration atoms)
target_include_directories(configuration-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
catch_discover_tests(configuration-test)
