use serde::{Deserialize, Serialize};

use crate::pos::{RelativeIndexType, RelativeVoxelPos, VoxelPos};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VoxelWorld {
    bodies: Vec<super::Voxel>,
}

impl VoxelWorld {
    pub fn from_world_and_min_pos(
        world: &crate::voxel_world::VoxelWorld,
        min_pos: RelativeVoxelPos,
    ) -> Self {
        let pos_to_rel = |pos: VoxelPos| {
            RelativeVoxelPos(
                pos.0
                    .zip(min_pos.0)
                    .map(|(pos, min)| RelativeIndexType::try_from(pos).unwrap() - min),
            )
        };

        Self {
            bodies: world
                .all_bodies()
                .map(|(body, pos)| (body, pos_to_rel(pos)).into())
                .collect(),
        }
    }
    pub fn from_world(world: &crate::voxel_world::VoxelWorld) -> Self {
        Self::from_world_and_min_pos(world, RelativeVoxelPos([0; 3]))
    }

    pub fn to_world_and_min_pos(
        &self,
    ) -> Result<
        (crate::voxel_world::VoxelWorld, RelativeVoxelPos),
        crate::voxel_world::InvalidVoxelWorldError,
    > {
        crate::voxel_world::VoxelWorld::from_bodies_rel_pos(
            self.bodies.iter().copied().map(Into::into),
        )
    }
}
