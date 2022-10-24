use crate::pos::VoxelPos;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VoxelWorld {
    bodies: Vec<super::Voxel>,
}

impl VoxelWorld {
    fn get_sizes(&self) -> VoxelPos {
        let mut sizes = [0; 3];
        for voxel in &self.bodies {
            let VoxelPos(voxel_pos) = voxel.pos;
            for (size, &voxel_i) in sizes.iter_mut().zip(voxel_pos.iter()) {
                *size = std::cmp::max(*size, voxel_i + 1);
            }
        }
        VoxelPos(sizes)
    }
}

impl From<&crate::voxel_world::VoxelWorld> for VoxelWorld {
    fn from(world: &crate::voxel_world::VoxelWorld) -> Self {
        Self {
            bodies: world.all_bodies().map(super::Voxel::from).collect(),
        }
    }
}
impl From<&VoxelWorld> for crate::voxel_world::VoxelWorld {
    fn from(world: &VoxelWorld) -> Self {
        let mut result = Self::new(world.get_sizes());
        for &voxel in &world.bodies {
            let (body, pos) = crate::voxel::VoxelBodyWithPos::from(voxel);
            *result.get_voxel_mut(pos).expect("Wrong size from world") =
                crate::voxel::Voxel::new_with_body(body);
        }
        result
    }
}
