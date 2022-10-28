mod graph;

pub use graph::ConnectivityGraph;

use crate::pos::VoxelPos;
use crate::voxel::VoxelBodyWithPos;
use crate::voxel_world::VoxelWorld;
use smallvec::SmallVec;

pub fn get_bodies_connected_to_body(
    body: VoxelBodyWithPos,
    world: &VoxelWorld,
) -> SmallVec<[VoxelBodyWithPos; 3]> {
    let (_, VoxelPos(body_pos)) = body;
    let connectors_dirs: [_; 3] = body.0.get_connectors_dirs();
    connectors_dirs
        .iter()
        .filter_map(|dir| {
            let other_pos = VoxelPos(dir.update_position(body_pos).ok()?);
            let other_body = world.get_body(other_pos)?;
            if other_body.get_connectors_dirs().contains(&dir.opposite()) {
                Some((other_body, other_pos))
            } else {
                None
            }
        })
        .collect()
}
