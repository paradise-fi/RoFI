use crate::*;

#[cxx::bridge(namespace = "rofi::voxel")]
mod ffi {
    extern "Rust" {
        fn voxel_reconfiguration(init_json: &str, goal_json: &str) -> Result<Vec<String>>;
    }
}

fn voxel_reconfiguration(init_json: &str, goal_json: &str) -> Result<Vec<String>, failure::Error> {
    let init = serde_json::from_str::<serde::VoxelWorld>(init_json)?;
    let goal = serde_json::from_str::<serde::VoxelWorld>(goal_json)?;

    let (init, _min_pos) = init.to_world_and_min_pos()?;
    let (goal, _min_pos) = goal.to_world_and_min_pos()?;

    let reconfig_sequence = reconfiguration::compute_reconfiguration_moves(&init, goal)?;

    Ok(reconfig_sequence
        .iter()
        .map(|world| serde_json::to_string(&serde::VoxelWorld::from_world(world)))
        .collect::<Result<Vec<_>, _>>()?)
}
