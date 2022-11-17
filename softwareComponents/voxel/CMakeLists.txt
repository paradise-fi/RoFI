cmake_minimum_required(VERSION 3.11)

add_library(voxel INTERFACE)
target_link_libraries(voxel INTERFACE atoms configuration nlohmann_json::nlohmann_json magic_enum)
target_include_directories(voxel INTERFACE include)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-voxel ${TEST_SRC})
target_link_libraries(test-voxel PRIVATE voxel Catch2WithMain configurationWithJson fmt)
