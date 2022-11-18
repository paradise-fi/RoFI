use crate::*;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[no_mangle]
#[allow(clippy::missing_safety_doc)]
unsafe extern "C" fn rust_free_cstring(rust_cstring: *mut i8) {
    if rust_cstring.is_null() {
        return;
    }
    let _ = CString::from_raw(rust_cstring);
}

#[no_mangle]
#[allow(clippy::missing_safety_doc)]
unsafe extern "C" fn compute_reconfiguration_moves(
    init_json: *const c_char,
    goal_json: *const c_char,
) -> *mut i8 {
    let init_json = CStr::from_ptr(init_json);
    let goal_json = CStr::from_ptr(goal_json);

    let result = voxel_reconfiguration_impl(init_json, goal_json);
    match result {
        Ok(value) => value.into_raw(),
        Err(err) => {
            eprintln!("Encountered error while reconfiguration: {}", err);
            std::ptr::null_mut()
        }
    }
}

fn voxel_reconfiguration_impl(
    init_json: &CStr,
    goal_json: &CStr,
) -> Result<CString, failure::Error> {
    let init = serde_json::from_str::<serde::VoxelWorld>(init_json.to_str()?)?;
    let goal = serde_json::from_str::<serde::VoxelWorld>(goal_json.to_str()?)?;

    let (init, _min_pos) = init.to_world_and_min_pos()?;
    let (goal, _min_pos) = goal.to_world_and_min_pos()?;

    let reconfig_sequence = reconfiguration::compute_reconfiguration_moves(&init, goal)?;

    Ok(CString::new(serde_json::to_string(
        &reconfig_sequence
            .iter()
            .map(|world| serde::VoxelWorld::from_world(world))
            .collect::<Vec<_>>(),
    )?)?)
}
