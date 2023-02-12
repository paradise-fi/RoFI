//! Compute a shortest path using the [breadth-first search algorithm](https://en.wikipedia.org/wiki/Breadth-first_search).

#[cfg(test)]
mod test;

use crate::reconfig::{all_next_worlds_norm, get_path_to, Error, FxHashMap};
use crate::reconfig::{log_counters, ADDED_WORLDS, NEXT_WORLDS_BEFORE_ADD, STEPS_COMPUTED};
use crate::voxel_world::{as_one_of_norm_eq_world, is_normalized, normalized_eq_worlds};
use crate::voxel_world::{check_voxel_world, NormVoxelWorld};
use std::assert_matches::{assert_matches, debug_assert_matches};
use std::collections::VecDeque;
use std::rc::Rc;
use std::sync::atomic;

type ParentMap<TWorld> = FxHashMap<Rc<TWorld>, Option<Rc<TWorld>>>;

pub fn compute_reconfig_path<TWorld>(init: TWorld, goal: TWorld) -> Result<Vec<Rc<TWorld>>, Error>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    assert_matches!(check_voxel_world(&init), Ok(()));
    assert_matches!(check_voxel_world(&goal), Ok(()));

    if init.all_voxels().count() != goal.all_voxels().count() {
        return Err(Error::VoxelCountDoesNotMatch);
    }

    let goal = as_one_of_norm_eq_world(goal);
    assert!(is_normalized(&goal));

    let parent_map = compute_reconfig_parents(&init, &goal).ok_or(Error::PathNotFound)?;

    log_counters();

    Ok(get_path_to(&goal, &parent_map, Clone::clone))
}

fn compute_reconfig_parents<TWorld>(init: &TWorld, goal: &TWorld) -> Option<ParentMap<TWorld>>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    debug_assert!(is_normalized(goal));
    debug_assert!(init.all_voxels().count() == goal.all_voxels().count());

    let mut init_worlds = normalized_eq_worlds(init).map(Rc::new).peekable();
    let init = init_worlds.peek().expect("No normalized variant").clone();
    let mut parent_map = init_worlds
        .map(|init| (init, None))
        .collect::<ParentMap<_>>();
    let mut worlds_to_visit = VecDeque::from([init]);

    if parent_map.contains_key(goal) {
        return Some(parent_map);
    }

    while let Some(current) = worlds_to_visit.pop_front() {
        debug_assert_matches!(check_voxel_world(current.as_ref()), Ok(()));
        debug_assert!(is_normalized(current.as_ref()));
        STEPS_COMPUTED.fetch_add(1, atomic::Ordering::Relaxed);
        for new_world in all_next_worlds_norm(current.as_ref()) {
            NEXT_WORLDS_BEFORE_ADD.fetch_add(1, atomic::Ordering::Relaxed);
            debug_assert_matches!(check_voxel_world(&new_world), Ok(()));
            debug_assert!(is_normalized(&new_world));

            if parent_map.contains_key(&new_world) {
                debug_assert!(
                    normalized_eq_worlds(&new_world)
                        .all(|norm_world| parent_map.contains_key(&norm_world)),
                    "parent map contains a normalized variant, but not itself"
                );
                continue;
            }

            debug_assert!(
                normalized_eq_worlds(&new_world)
                    .all(|norm_world| !parent_map.contains_key(&norm_world)),
                "parent map contains itself but not some normalized variant"
            );

            let mut norm_worlds = normalized_eq_worlds(&new_world).map(Rc::new).peekable();
            let new_world = norm_worlds.peek().expect("No normalized variant").clone();
            debug_assert!(is_normalized(new_world.as_ref()));

            parent_map.extend(norm_worlds.map(|norm_world| (norm_world, Some(current.clone()))));

            if parent_map.contains_key(goal) {
                return Some(parent_map);
            }

            worlds_to_visit.push_back(new_world);
            ADDED_WORLDS.fetch_add(1, atomic::Ordering::Relaxed);
        }
    }
    None
}
