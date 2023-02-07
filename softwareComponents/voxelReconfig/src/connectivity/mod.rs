pub mod graph;

pub use graph::ConnectivityGraph;

use crate::voxel::PosVoxel;
use crate::voxel_world::VoxelWorld;
use smallvec::SmallVec;

pub fn get_bodies_connected_to<TWorld: VoxelWorld>(
    pos_voxel: PosVoxel<TWorld::IndexType>,
    world: &TWorld,
) -> SmallVec<[PosVoxel<TWorld::IndexType>; 3]> {
    let (pos, voxel) = pos_voxel;
    let connectors_dirs: [_; 3] = voxel.get_connectors_dirs();
    connectors_dirs
        .iter()
        .filter_map(|dir| {
            let other_pos = dir.update_position(pos.as_array()).into();
            let other_voxel = world.get_voxel(other_pos)?;
            if other_voxel.get_connectors_dirs().contains(&dir.opposite()) {
                Some((other_pos, other_voxel))
            } else {
                None
            }
        })
        .collect()
}
