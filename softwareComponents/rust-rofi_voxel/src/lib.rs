#![feature(array_methods, array_zip, assert_matches)]

pub mod atoms;
pub mod pos;
pub mod voxel;
pub mod voxel_world;

pub mod serde;

pub mod connectivity;
pub mod module_move;
pub mod module_repr;
pub mod reconfiguration;

#[cfg(feature = "cpp_json_bindings")]
mod cpp_json_bindings;
