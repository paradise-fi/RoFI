cmake_minimum_required(VERSION 3.15)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
rofi_register_corrosion_lib(rofi_voxel)

add_cargo_package_tests(test-rofi_voxel MANIFEST_PATH Cargo.toml)
