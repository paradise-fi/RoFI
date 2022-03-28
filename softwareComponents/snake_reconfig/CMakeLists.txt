cmake_minimum_required(VERSION 3.10)
project(rofi)


add_library(snakeStructs Snake_structs.h Snake_structs.cpp)
target_link_libraries(snakeStructs PUBLIC configuration legacy-configuration)

add_library(snakeAlgorithms Snake_algorithms.h Snake_algorithms.cpp)
target_link_libraries(snakeAlgorithms PUBLIC configuration legacy-configuration reconfig snakeStructs nlohmann_json::nlohmann_json)

add_executable(snakeReconfig main.cpp)
target_link_libraries(snakeReconfig PUBLIC
    configuration legacy-configuration reconfig snakeStructs snakeAlgorithms dimcli)

add_executable(test-snakeReconfig test/test.cpp)
target_link_libraries(test-snakeReconfig PUBLIC configuration legacy-configuration reconfig Catch2WithMain snakeAlgorithms)
target_include_directories(test-snakeReconfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
