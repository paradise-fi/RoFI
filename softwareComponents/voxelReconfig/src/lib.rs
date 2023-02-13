#![feature(
    array_methods,
    array_zip,
    assert_matches,
    associated_type_defaults,
    is_some_and,
    map_try_insert
)]

pub mod atoms;
pub mod pos;
pub mod voxel;
pub mod voxel_world;

pub mod serde;

pub mod connectivity;
pub mod module_move;
pub mod module_repr;
pub mod reconfig;
