cmake_minimum_required(VERSION 3.15)

add_rust_executable(rofi-voxel_main)
add_rust_executable(rofi-voxel_full_traversal_bfs)
add_rust_executable(rofi-voxel_normalized)
add_rust_executable(rofi-voxel_one_step)

add_rust_tests(test-rofi-voxel)

add_executable_script(rofi-voxel DEPENDS rofi-convert rofi-voxel_main rofi-tool)
