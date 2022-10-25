cmake_minimum_required(VERSION 3.15)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
rofi_register_corrosion_bin(rofi-voxel_reconfig)

add_cargo_package_tests(test-rofi-voxel_reconfig MANIFEST_PATH Cargo.toml)

add_executable_script(rofi-voxel DEPENDS rofi-convert rofi-voxel_reconfig)
