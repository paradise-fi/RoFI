cmake_minimum_required(VERSION 3.15)

add_rust_library(rofi_voxel FEATURES cpp_json_bindings)
add_rust_tests(test-rofi_voxel ALL_FEATURES)


set(BINDING_SRCS
    binding_src/voxel_reconfig_binding.cpp
)
add_library(voxel_reconfig ${BINDING_SRCS})
target_link_libraries(voxel_reconfig PRIVATE rofi_voxel configurationWithJson)
target_link_libraries(voxel_reconfig PUBLIC atoms voxel)
target_include_directories(voxel_reconfig PUBLIC binding_include)

file(GLOB BINDING_TEST_SRC binding_test/*.cpp)
add_executable(test-binding_voxel_reconfig ${BINDING_TEST_SRC})
target_link_libraries(test-binding_voxel_reconfig PRIVATE Catch2WithMain voxel_reconfig)
