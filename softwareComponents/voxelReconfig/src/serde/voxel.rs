use crate::atoms;
use crate::pos::RelativeVoxelPos;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Voxel {
    pub pos: RelativeVoxelPos,
    pub other_body_dir: atoms::Direction,
    pub is_shoe_rotated: bool,
    pub joint_pos: crate::voxel::body::JointPosition,
}

impl From<Voxel> for (crate::voxel::VoxelBody, RelativeVoxelPos) {
    fn from(value: Voxel) -> Self {
        let Voxel {
            pos,
            other_body_dir,
            is_shoe_rotated,
            joint_pos,
        } = value;
        (
            crate::voxel::VoxelBody::new_with(other_body_dir, is_shoe_rotated, joint_pos),
            pos,
        )
    }
}
impl From<(crate::voxel::VoxelBody, RelativeVoxelPos)> for Voxel {
    fn from(value: (crate::voxel::VoxelBody, RelativeVoxelPos)) -> Self {
        let (value, pos) = value;
        Self {
            pos,
            other_body_dir: value.other_body_dir(),
            is_shoe_rotated: value.is_shoe_rotated(),
            joint_pos: value.joint_pos(),
        }
    }
}
