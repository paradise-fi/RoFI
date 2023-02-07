use crate::pos::Pos;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VoxelWorld<TIndex: num::Num + Clone> {
    bodies: Vec<super::Voxel<TIndex>>,
}

impl<TIndex: num::Num + Clone> VoxelWorld<TIndex> {
    pub fn from_world(world: &impl crate::voxel_world::NormVoxelWorld<IndexType = TIndex>) -> Self {
        Self {
            bodies: world.all_voxels().map(Into::into).collect(),
        }
    }

    pub fn to_world_and_min_pos<RWorld>(
        &self,
    ) -> Result<(RWorld, Pos<TIndex>), crate::voxel_world::InvalidVoxelWorldError<TIndex>>
    where
        RWorld: crate::voxel_world::NormVoxelWorld<IndexType = TIndex>,
        TIndex: std::fmt::Debug + Ord + Copy,
    {
        RWorld::from_voxels(self.bodies.iter().copied().map(Into::into))
    }
}
