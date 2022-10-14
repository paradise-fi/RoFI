mod trait_impls;
pub mod voxel;

use crate::pos::VoxelPos;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VoxelWorld {
    modules: Vec<self::voxel::Voxel>,
}

impl VoxelWorld {
    fn get_sizes(&self) -> VoxelPos {
        let mut sizes = [0; 3];
        for module in &self.modules {
            let VoxelPos(module_pos) = module.pos;
            for (size, &voxel_i) in sizes.iter_mut().zip(module_pos.iter()) {
                *size = std::cmp::max(*size, voxel_i);
            }
        }
        VoxelPos(sizes)
    }
}

impl From<&crate::voxel_world::VoxelWorld> for VoxelWorld {
    fn from(world: &crate::voxel_world::VoxelWorld) -> Self {
        Self {
            modules: world.all_bodies().map(self::voxel::Voxel::from).collect(),
        }
    }
}
impl From<&VoxelWorld> for crate::voxel_world::VoxelWorld {
    fn from(world: &VoxelWorld) -> Self {
        let mut result = Self::new(world.get_sizes());
        for &module in &world.modules {
            let (body, pos) = crate::voxel::VoxelBodyWithPos::from(module);
            *result.get_voxel_mut(pos).expect("Wrong size from world") =
                crate::voxel::Voxel::new_with_body(body);
        }
        result
    }
}
