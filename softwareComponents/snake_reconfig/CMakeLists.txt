cmake_minimum_required(VERSION 3.10)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_library(snakeStructs Snake_structs.h Snake_structs.cpp)
target_link_libraries(snakeStructs PUBLIC configuration generators)

add_library(snakeAlgorithms Snake_algorithms.h Snake_algorithms.cpp)
target_link_libraries(snakeAlgorithms PUBLIC configuration generators reconfig snakeStructs nlohmann_json::nlohmann_json)

add_executable(snakeReconfig main.cpp)
target_link_libraries(snakeReconfig PUBLIC
    configuration generators reconfig snakeStructs snakeAlgorithms dimcli)

add_executable(test-snakeReconfig test/main.cpp test/test.cpp)
target_link_libraries(test-snakeReconfig PUBLIC configuration generators reconfig Catch2::Catch2 snakeAlgorithms)
target_include_directories(test-snakeReconfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
