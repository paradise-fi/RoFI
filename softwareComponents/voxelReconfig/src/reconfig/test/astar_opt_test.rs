use super::{validate_norm_voxel_world, validate_reconfig_path};
use crate::algs::astar::opt::compute_path;
use crate::atoms::{Axis, Direction};
use crate::reconfig::metric::{naive::NaiveMetric, ZeroMetric};
use crate::reconfig::voxel_worlds_graph::VoxelWorldsGraph;
use crate::voxel::{JointPosition, Voxel};
use crate::voxel_world::as_one_of_norm_eq_world;
use crate::voxel_world::impls::{MapVoxelWorld, MatrixVoxelWorld, SortvecVoxelWorld};
use crate::voxel_world::NormVoxelWorld;
use std::rc::Rc;

#[test]
pub fn test_reconfig_no_steps_map() {
    test_reconfig_no_steps::<MapVoxelWorld<i8>>();
}
#[test]
pub fn test_reconfig_no_steps_matrix() {
    test_reconfig_no_steps::<MatrixVoxelWorld<i8>>();
}
#[test]
pub fn test_reconfig_no_steps_sortvec() {
    test_reconfig_no_steps::<SortvecVoxelWorld<i8>>();
}
fn test_reconfig_no_steps<TWorld>()
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash + Clone + std::fmt::Debug,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    let (world, _) = TWorld::from_voxels([
        (
            [num::zero(); 3].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, true),
                true,
                JointPosition::Plus90,
            ),
        ),
        (
            [num::one(), num::zero(), num::zero()].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, false),
                false,
                JointPosition::Minus90,
            ),
        ),
    ])
    .unwrap();
    validate_norm_voxel_world(&world);

    let world = Rc::new(as_one_of_norm_eq_world(world));
    let expected_path = &[world.clone()];

    let result = compute_path::<VoxelWorldsGraph<TWorld>, ZeroMetric>(&world, &world).unwrap();
    validate_reconfig_path(&result, expected_path);

    let result = compute_path::<VoxelWorldsGraph<TWorld>, NaiveMetric<_>>(&world, &world).unwrap();
    validate_reconfig_path(&result, expected_path);
}

#[test]
pub fn test_reconfig_one_step_map() {
    test_reconfig_one_step::<MapVoxelWorld<i8>>();
}
#[test]
pub fn test_reconfig_one_step_matrix() {
    test_reconfig_one_step::<MatrixVoxelWorld<i8>>();
}
#[test]
pub fn test_reconfig_one_step_sortvec() {
    test_reconfig_one_step::<SortvecVoxelWorld<i8>>();
}
pub fn test_reconfig_one_step<TWorld>()
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash + Clone + std::fmt::Debug,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    let (init_world, _) = TWorld::from_voxels([
        (
            [num::zero(); 3].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, true),
                true,
                JointPosition::Plus90,
            ),
        ),
        (
            [num::one(), num::zero(), num::zero()].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, false),
                false,
                JointPosition::Minus90,
            ),
        ),
    ])
    .unwrap();
    validate_norm_voxel_world(&init_world);
    let (goal_world, _) = TWorld::from_voxels([
        (
            [num::zero(); 3].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, true),
                true,
                JointPosition::Plus90,
            ),
        ),
        (
            [num::one(), num::zero(), num::zero()].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, false),
                true,
                JointPosition::Minus90,
            ),
        ),
    ])
    .unwrap();
    validate_norm_voxel_world(&goal_world);

    let init_world = Rc::new(as_one_of_norm_eq_world(init_world));
    let goal_world = Rc::new(as_one_of_norm_eq_world(goal_world));
    let expected_path = &[init_world.clone(), goal_world.clone()];

    let result =
        compute_path::<VoxelWorldsGraph<TWorld>, ZeroMetric>(&init_world, &goal_world).unwrap();
    validate_reconfig_path(&result, expected_path);

    let result =
        compute_path::<VoxelWorldsGraph<TWorld>, NaiveMetric<_>>(&init_world, &goal_world).unwrap();
    validate_reconfig_path(&result, expected_path);
}
