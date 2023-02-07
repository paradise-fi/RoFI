use crate::atoms;
use crate::pos::Pos;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Voxel<TIndex: num::Num + Clone> {
    pub pos: Pos<TIndex>,
    pub body_dir: atoms::Direction,
    pub shoe_rotated: bool,
    pub joint_pos: crate::voxel::JointPosition,
}

impl<TIndex: num::Num + Clone> From<Voxel<TIndex>> for crate::voxel::PosVoxel<TIndex> {
    fn from(value: Voxel<TIndex>) -> Self {
        let Voxel {
            pos,
            body_dir,
            shoe_rotated,
            joint_pos,
        } = value;
        (
            pos,
            crate::voxel::Voxel::new_with(body_dir, shoe_rotated, joint_pos),
        )
    }
}
impl<TIndex: num::Num + Clone> From<crate::voxel::PosVoxel<TIndex>> for Voxel<TIndex> {
    fn from(value: crate::voxel::PosVoxel<TIndex>) -> Self {
        let (pos, voxel) = value;
        Self {
            pos,
            body_dir: voxel.body_dir(),
            shoe_rotated: voxel.shoe_rotated(),
            joint_pos: voxel.joint_pos(),
        }
    }
}
