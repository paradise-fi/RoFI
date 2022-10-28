use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VoxelWorld {
    bodies: Vec<super::Voxel>,
}

impl From<&crate::voxel_world::VoxelWorld> for VoxelWorld {
    fn from(world: &crate::voxel_world::VoxelWorld) -> Self {
        Self {
            bodies: world.all_bodies().map(Into::into).collect(),
        }
    }
}
impl TryFrom<&VoxelWorld> for crate::voxel_world::VoxelWorld {
    type Error = crate::voxel_world::InvalidVoxelWorldError;

    fn try_from(world: &VoxelWorld) -> Result<Self, Self::Error> {
        Self::from_bodies(world.bodies.iter().copied().map(Into::into))
    }
}
