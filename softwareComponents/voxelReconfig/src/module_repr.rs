use crate::pos::VoxelPos;
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::{VoxelBody, VoxelBodyWithPos};
use crate::voxel_world::VoxelWorld;

pub fn is_module_repr(body: VoxelBody) -> bool {
    body.other_body_dir().is_positive()
}

pub fn get_module_repr_pos(body_with_pos: VoxelBodyWithPos) -> VoxelPos {
    if is_module_repr(body_with_pos.0) {
        body_with_pos.1
    } else {
        get_neighbour_pos(body_with_pos).unwrap()
    }
}

pub fn get_all_module_reprs(world: &VoxelWorld) -> impl Iterator<Item = VoxelBodyWithPos> + '_ {
    world.all_bodies().filter(|&(body, _)| is_module_repr(body))
}

pub fn get_bodies(module: VoxelBodyWithPos, world: &VoxelWorld) -> [VoxelBodyWithPos; 2] {
    debug_assert!(is_module_repr(module.0));
    let other_pos = get_neighbour_pos(module).unwrap();
    let other_body = world
        .get_body(other_pos)
        .expect("Missing other body in world");

    [module, (other_body, other_pos)]
}
