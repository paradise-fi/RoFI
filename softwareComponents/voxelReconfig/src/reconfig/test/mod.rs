mod test_paths;

use crate::algs;
use crate::algs::{astar::AstarAlgInfo, bfs::BfsAlgInfo};
use crate::pos::{minimal_pos_hull, SizeRanges};
use crate::reconfig::metric::{naive::NaiveMetric, ZeroMetric};
use crate::reconfig::voxel_worlds_graph::VoxelWorldsGraph;
use crate::voxel::PosVoxel;
use crate::voxel_world::impls::{MapVoxelWorld, MatrixVoxelWorld, SortvecVoxelWorld};
use crate::voxel_world::{
    as_one_of_norm_eq_world_with_rot, NormVoxelWorld, VoxelWorld, WorldRotation,
};
use iter_fixed::IntoIteratorFixed;
use multicode::multicode;
use std::assert_matches::assert_matches;
use std::rc::Rc;
use test_paths::{no_steps_path, one_step_path};

pub fn validate_voxel_world(world: &impl VoxelWorld) {
    for (pos, voxel) in world.all_voxels() {
        if !pos
            .as_array()
            .into_iter_fixed()
            .zip(world.size_ranges().as_ranges_array())
            .into_iter()
            .all(|(pos, size_range)| size_range.contains(&pos))
        {
            panic!(
                "Position is out of bounds (pos={pos:?}, size_ranges={:?})",
                world.size_ranges()
            );
        }
        assert_eq!(world.get_voxel(pos), Some(voxel));
    }

    assert_eq!(
        world.size_ranges(),
        minimal_pos_hull(world.all_voxels().map(|(pos, _)| pos))
    );
}
pub fn validate_norm_voxel_world(world: &impl NormVoxelWorld) {
    assert_eq!(world.size_ranges(), SizeRanges::from_sizes(world.sizes()));
    validate_voxel_world(world);
}

fn validate_reconfig_path<TWorld>(path: &[Rc<TWorld>], expected_path: &[Rc<TWorld>])
where
    TWorld: NormVoxelWorld + Eq + std::fmt::Debug,
{
    path.iter()
        .map(AsRef::as_ref)
        .for_each(validate_norm_voxel_world);
    assert_eq!(path, expected_path);
}

pub fn test_path_exact<TWorld, IWorlds>(
    compute_path: impl FnOnce(&TWorld, &TWorld) -> Result<Vec<Rc<TWorld>>, algs::Error>,
    path: IWorlds,
) where
    TWorld: NormVoxelWorld + std::fmt::Debug + Eq,
    IWorlds: IntoIterator<Item = Vec<PosVoxel<TWorld::IndexType>>>,
{
    let path = path
        .into_iter()
        .map(|voxels| {
            let world = TWorld::from_voxels(voxels);
            assert_matches!(world, Ok(_));
            let (world, origin) = world.unwrap();
            assert_eq!(origin.as_array(), [num::zero(); 3]);
            validate_norm_voxel_world(&world);
            let (world, rotation) = as_one_of_norm_eq_world_with_rot(world);
            assert_eq!(rotation, WorldRotation::identity());
            let world = Rc::new(world);
            validate_norm_voxel_world(world.as_ref());
            world
        })
        .collect::<Vec<_>>();
    assert!(!path.is_empty());

    let result = compute_path(path.first().unwrap(), path.last().unwrap());
    assert_matches!(result, Ok(_));
    validate_reconfig_path(&result.unwrap(), &path);
}

#[multicode(TAlgInfo, [
    bfs: BfsAlgInfo<_>;
    astar_zero: AstarAlgInfo<_, ZeroMetric>;
    astar_naive: AstarAlgInfo<_, NaiveMetric<_>>;
])]
#[multicode(COMPUTE_PATH, [
    onedir: algs::onedir::compute_path;
    bidir: algs::bidir::compute_path;
])]
#[multicode(TWorld, [
    matrix_i8: MatrixVoxelWorld<i8>;
    map_i8: MapVoxelWorld<i8>;
    sortvec_i8: SortvecVoxelWorld<i8>;
])]
#[multicode(PATH, [
    no_steps: no_steps_path();
    one_step: one_step_path();
])]
#[test]
fn test_exact() {
    test_path_exact(COMPUTE_PATH::<VoxelWorldsGraph<TWorld>, TAlgInfo>, PATH);
}
