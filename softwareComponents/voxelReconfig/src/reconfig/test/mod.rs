mod astar_opt_test;
mod astar_test;
mod bfs_test;

use crate::pos::{minimal_pos_hull, SizeRanges};
use crate::voxel_world::{NormVoxelWorld, VoxelWorld};
use iter_fixed::IntoIteratorFixed;
use std::rc::Rc;

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
