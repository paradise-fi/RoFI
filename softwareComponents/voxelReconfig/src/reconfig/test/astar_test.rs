use crate::algs::astar::compute_path;
use crate::atoms::{Axis, Direction};
use crate::reconfig::heuristic::Heuristic;
use crate::reconfig::test::validate_norm_voxel_world;
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
    let world = as_one_of_norm_eq_world(world);

    for heuristic in [Heuristic::Zero, Heuristic::Naive] {
        let heuristic = heuristic.get_fn(&world);
        let result = compute_path::<VoxelWorldsGraph<_>, _>(&world, &world, heuristic).unwrap();
        assert_eq!(result.len(), 1);
        result
            .iter()
            .map(AsRef::<TWorld>::as_ref)
            .for_each(validate_norm_voxel_world);
        assert_eq!(&result, &vec![Rc::new(world.clone())]);
    }
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

    for heuristic in [Heuristic::Zero, Heuristic::Naive] {
        let heuristic = heuristic.get_fn(&goal_world);
        let result =
            compute_path::<VoxelWorldsGraph<_>, _>(&init_world, &goal_world, heuristic).unwrap();
        assert_eq!(result.len(), 2);
        result
            .iter()
            .map(AsRef::<TWorld>::as_ref)
            .for_each(validate_norm_voxel_world);
        assert_eq!(
            &result,
            &vec![Rc::new(init_world.clone()), Rc::new(goal_world.clone())]
        );
    }
}
