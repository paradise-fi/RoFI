cmake_minimum_required(VERSION 3.15)

add_rust_executable(rofi-voxel_reconfig)
add_rust_tests(test-rofi-voxel_reconfig)

add_executable_script(rofi-voxel DEPENDS rofi-convert rofi-voxel_reconfig rofi-tool)
