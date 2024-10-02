cmake_minimum_required(VERSION 3.11)

add_library(voxel_serialization INTERFACE)
target_link_libraries(voxel_serialization INTERFACE atoms configuration nlohmann_json::nlohmann_json magic_enum)
target_include_directories(voxel_serialization INTERFACE include)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-voxel_serialization ${TEST_SRC})
target_link_libraries(test-voxel_serialization PRIVATE voxel_serialization Catch2WithMain configurationWithJson fmt)
