cmake_minimum_required(VERSION 3.6)

file(GLOB_RECURSE SRC src/*.cpp)

add_library(taskDistribution ${SRC})
target_link_libraries(taskDistribution PUBLIC leadership networking)
target_include_directories(taskDistribution PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(taskDistribution PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
