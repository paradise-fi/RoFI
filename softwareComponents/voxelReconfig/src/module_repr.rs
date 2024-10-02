use crate::pos::Pos;
use crate::voxel::{get_other_body_pos, PosVoxel, Voxel};
use crate::voxel_world::VoxelWorld;

pub fn is_module_repr(voxel: Voxel) -> bool {
    voxel.body_dir().is_positive()
}

pub fn get_module_repr_pos<TIndex: num::Signed + Copy>(pos_voxel: PosVoxel<TIndex>) -> Pos<TIndex> {
    if is_module_repr(pos_voxel.1) {
        pos_voxel.0
    } else {
        get_other_body_pos(pos_voxel)
    }
}

pub fn get_all_module_reprs<TWorld: VoxelWorld>(
    world: &TWorld,
) -> impl '_ + Iterator<Item = PosVoxel<TWorld::IndexType>> {
    world
        .all_voxels()
        .filter(|&(_, voxel)| is_module_repr(voxel))
}

pub fn get_other_body<TWorld: VoxelWorld>(
    pos_voxel: PosVoxel<TWorld::IndexType>,
    world: &TWorld,
) -> Result<PosVoxel<TWorld::IndexType>, String> {
    let other_pos = get_other_body_pos(pos_voxel);
    let other_voxel = world
        .get_voxel(other_pos)
        .ok_or("Missing other body in world")?;

    Ok((other_pos, other_voxel))
}
