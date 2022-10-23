cmake_minimum_required(VERSION 3.15)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
add_cargo_package_tests(test-rust-rofi_voxel MANIFEST_PATH Cargo.toml)
