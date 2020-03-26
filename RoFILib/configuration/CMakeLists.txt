cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(configuration STATIC Configuration.cpp Configuration.h)
target_include_directories(configuration PUBLIC ${ARMADILLO_INCLUDE_DIRS})
target_include_directories(configuration PUBLIC .)
target_link_libraries(configuration PRIVATE ${ARMADILLO_LIBRARIES})

add_library(generators STATIC Generators.cpp Generators.h)
target_include_directories(generators PUBLIC .)
target_link_libraries(generators PUBLIC configuration)
