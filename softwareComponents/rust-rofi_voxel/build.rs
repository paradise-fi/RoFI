fn build_cpp_json_bindings() {
    cxx_build::bridge("src/cpp_json_bindings.rs")
        .shared_flag(true)
        .out_dir(std::env::var_os("CXX_OUT_DIR").expect("CXX_OUT_DIR is not set"))
        .compile("rofi_voxel_cpp_json_bindings");

    println!("cargo:rerun-if-changed=src/cpp_json_bindings.rs");
}

fn main() {
    build_cpp_json_bindings();
}
