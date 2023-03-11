#![feature(
    array_methods,
    assert_matches,
    associated_type_defaults,
    map_try_insert,
    impl_trait_in_assoc_type
)]

pub mod atoms;
pub mod pos;
pub mod voxel;
pub mod voxel_world;

pub mod serde;

pub mod algs;
pub mod connectivity;
pub mod counters;
pub mod module_move;
pub mod module_repr;
pub mod reconfig;
