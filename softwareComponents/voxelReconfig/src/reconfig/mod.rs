use crate::connectivity::ConnectivityGraph;
use crate::module_move::Move;
use crate::module_repr::is_module_repr;
use crate::module_repr::{get_all_module_reprs, get_other_body};
use crate::voxel_world::NormVoxelWorld;
use crate::voxel_world::{as_one_of_norm_eq_world, normalized_eq_worlds};
use crate::voxel_world::{check_voxel_world, is_normalized};
use std::assert_matches::{assert_matches, debug_assert_matches};
use std::collections::{HashMap, VecDeque};
use std::rc::Rc;
use std::sync::atomic::{self, AtomicU64};

pub static ALL_WORLDS: AtomicU64 = AtomicU64::new(0);
pub static ADDED_WORLDS: AtomicU64 = AtomicU64::new(0);
pub static NEXT_WORLDS_BEFORE_ADD: AtomicU64 = AtomicU64::new(0);
pub static COLLIDED_WORLDS: AtomicU64 = AtomicU64::new(0);
pub static STEPS_COMPUTED: AtomicU64 = AtomicU64::new(0);

#[derive(Debug, Clone, Copy, amplify::Display, amplify::Error)]
#[display(doc_comments)]
pub enum Error {
    /// Voxel count doesn't match
    VoxelCountDoesNotMatch,
    /// Reconfiguration path not found
    PathNotFound,
}

fn get_path_to<TWorld: Eq + std::hash::Hash>(
    goal: &TWorld,
    parent_worlds: &HashMap<Rc<TWorld>, Option<Rc<TWorld>>>,
) -> Vec<Rc<TWorld>> {
    assert!(parent_worlds.contains_key(goal), "Missing goal parent");

    let mut parent = Some(parent_worlds.get_key_value(goal).unwrap().0.clone());
    let mut path = std::iter::from_fn(|| {
        let child = parent.as_ref()?;
        let new_parent = parent_worlds.get(child).expect("Missing prev node").clone();
        std::mem::replace(&mut parent, new_parent)
    })
    .collect::<Vec<_>>();

    path.reverse();
    path
}

/// Returns all possible next worlds
///
/// The returned world is NOT normalized
pub fn all_next_worlds_not_norm<TWorld>(world: &TWorld) -> impl Iterator<Item = TWorld> + '_
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    let graph = Rc::new(ConnectivityGraph::compute_from(world));
    get_all_module_reprs(world)
        .map(move |module| (module, graph.clone()))
        .flat_map(move |(module, graph)| {
            assert!(is_module_repr(module.1));
            let other_body = get_other_body(module, world).unwrap();
            graph
                .all_cuts_by_module(module)
                .map(Rc::new)
                .flat_map(move |split| {
                    Move::all_possible_moves(module.1, other_body.1).flat_map(move |module_move| {
                        let result = module_move.apply(module, split.clone().as_ref());
                        ALL_WORLDS.fetch_add(1, atomic::Ordering::Relaxed);
                        if result.is_none() {
                            COLLIDED_WORLDS.fetch_add(1, atomic::Ordering::Relaxed);
                        }
                        result
                    })
                })
        })
}

/// Returns all possible next worlds
///
/// The returned world is normalized (one of possible eq norm worlds)
pub fn all_next_worlds_norm<TWorld>(world: &TWorld) -> impl Iterator<Item = TWorld> + '_
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    all_next_worlds_not_norm(world).map(as_one_of_norm_eq_world)
}

fn find_parents_from_to<TWorld>(
    init: &TWorld,
    goal: &TWorld,
) -> Result<HashMap<Rc<TWorld>, Option<Rc<TWorld>>>, Error>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    assert!(is_normalized(goal));

    if init.all_voxels().count() != goal.all_voxels().count() {
        return Err(Error::VoxelCountDoesNotMatch);
    }

    let mut parent_worlds = normalized_eq_worlds(init)
        .map(|init| (Rc::new(init), None))
        .collect::<HashMap<_, _>>();

    if parent_worlds.contains_key(goal) {
        return Ok(parent_worlds);
    }

    assert!(
        !parent_worlds.is_empty(),
        "Init has to have normalized variant"
    );
    let init = parent_worlds.keys().next().unwrap().clone();
    let mut worlds_to_visit = VecDeque::from([init]);

    while let Some(current) = worlds_to_visit.pop_front() {
        debug_assert_matches!(check_voxel_world(current.as_ref()), Ok(()));
        debug_assert!(is_normalized(current.as_ref()));
        STEPS_COMPUTED.fetch_add(1, atomic::Ordering::Relaxed);

        for new_world in all_next_worlds_norm(current.as_ref()) {
            NEXT_WORLDS_BEFORE_ADD.fetch_add(1, atomic::Ordering::Relaxed);
            debug_assert_matches!(check_voxel_world(&new_world), Ok(()));
            debug_assert!(is_normalized(&new_world));
            if parent_worlds.contains_key(&new_world) {
                debug_assert!(
                    normalized_eq_worlds(&new_world)
                        .all(|norm_world| parent_worlds.contains_key(&norm_world)),
                    "parent_worlds contains a normalized variant, but not itself"
                );
                continue;
            }

            debug_assert!(
                normalized_eq_worlds(&new_world)
                    .all(|norm_world| !parent_worlds.contains_key(&norm_world)),
                "parent_worlds contains itself but not some normalized variant"
            );

            parent_worlds.extend(
                normalized_eq_worlds(&new_world)
                    .map(|norm_world| (Rc::new(norm_world), Some(current.clone()))),
            );

            if parent_worlds.contains_key(goal) {
                return Ok(parent_worlds);
            }

            debug_assert!(is_normalized(&new_world));
            // Get the world as Rc from the parent_worlds
            let new_world = parent_worlds
                .get_key_value(&new_world)
                .expect("Has to contain new_world")
                .0
                .clone();

            worlds_to_visit.push_back(new_world);
            ADDED_WORLDS.fetch_add(1, atomic::Ordering::Relaxed);
        }
    }

    Err(Error::PathNotFound)
}

pub fn compute_reconfig_path<TWorld>(init: &TWorld, goal: TWorld) -> Result<Vec<Rc<TWorld>>, Error>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    assert_matches!(check_voxel_world(init), Ok(()));
    assert_matches!(check_voxel_world(&goal), Ok(()));

    let goal = as_one_of_norm_eq_world(goal);

    let parent_worlds = find_parents_from_to(init, &goal)?;

    log_counters();

    Ok(get_path_to(&goal, &parent_worlds))
}

pub fn log_counters() {
    let true_cuts = crate::connectivity::graph::TRUE_CUTS.load(atomic::Ordering::Relaxed);
    let false_cuts = crate::connectivity::graph::FALSE_CUTS.load(atomic::Ordering::Relaxed);
    log::info!("ALL_CUTS: {}", true_cuts + false_cuts);
    log::info!("TRUE_CUTS: {}", true_cuts);
    log::info!("FALSE_CUTS: {}", false_cuts);

    log::info!("ALL_WORLDS: {}", ALL_WORLDS.load(atomic::Ordering::Relaxed));
    log::info!(
        "COLLIDED_WORLDS: {}",
        COLLIDED_WORLDS.load(atomic::Ordering::Relaxed)
    );
    log::info!(
        "STEPS_COMPUTED: {}",
        STEPS_COMPUTED.load(atomic::Ordering::Relaxed)
    );
    log::info!(
        "NEXT_WORLDS_BEFORE_ADD: {}",
        NEXT_WORLDS_BEFORE_ADD.load(atomic::Ordering::Relaxed)
    );
    log::info!(
        "ADDED_WORLDS: {}",
        ADDED_WORLDS.load(atomic::Ordering::Relaxed)
    );
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::atoms::{Axis, Direction};
    use crate::pos::{minimal_pos_hull, SizeRanges};
    use crate::voxel::{JointPosition, Voxel};
    use crate::voxel_world::impls::{MapVoxelWorld, MatrixVoxelWorld, SortvecVoxelWorld};
    use crate::voxel_world::VoxelWorld;

    pub fn validate_voxel_world(world: &impl VoxelWorld) {
        for (pos, voxel) in world.all_voxels() {
            if !pos
                .as_array()
                .zip(world.size_ranges().as_ranges_array())
                .iter()
                .all(|(pos, size_range)| size_range.contains(pos))
            {
                panic!(
                    "Position is out of bounds (pos={pos:?}, size_ranges={:?})",
                    world.size_ranges()
                );
            }
            assert_eq!(world.get_voxel(pos), Some(voxel));
        }

        assert_eq!(
            world.size_ranges(),
            minimal_pos_hull(world.all_voxels().map(|(pos, _)| pos))
        );
    }
    pub fn validate_norm_voxel_world(world: &impl NormVoxelWorld) {
        assert_eq!(world.size_ranges(), SizeRanges::from_sizes(world.sizes()));
        validate_voxel_world(world);
    }

    #[test]
    pub fn test_reconfig_no_steps_map() {
        test_reconfig_no_steps::<MapVoxelWorld<i8>>();
    }
    #[test]
    pub fn test_reconfig_no_steps_matrix() {
        test_reconfig_no_steps::<MatrixVoxelWorld<i8>>();
    }
    #[test]
    pub fn test_reconfig_no_steps_sortvec() {
        test_reconfig_no_steps::<SortvecVoxelWorld<i8>>();
    }
    pub fn test_reconfig_no_steps<TWorld>()
    where
        TWorld: NormVoxelWorld + Eq + std::hash::Hash + Clone,
        TWorld::IndexType: num::Integer + std::hash::Hash,
    {
        let (world, _) = TWorld::from_voxels([
            (
                [num::zero(); 3].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [num::one(), num::zero(), num::zero()].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, false),
                    false,
                    JointPosition::Minus90,
                ),
            ),
        ])
        .unwrap();
        validate_norm_voxel_world(&world);

        let result = compute_reconfig_path(&world, world.clone()).unwrap();
        assert_eq!(result.len(), 1);
        result
            .iter()
            .map(AsRef::<TWorld>::as_ref)
            .for_each(validate_norm_voxel_world);
    }

    #[test]
    pub fn test_reconfig_one_step_map() {
        test_reconfig_one_step::<MapVoxelWorld<i8>>();
    }
    #[test]
    pub fn test_reconfig_one_step_matrix() {
        test_reconfig_one_step::<MatrixVoxelWorld<i8>>();
    }
    #[test]
    pub fn test_reconfig_one_step_sortvec() {
        test_reconfig_one_step::<SortvecVoxelWorld<i8>>();
    }
    pub fn test_reconfig_one_step<TWorld>()
    where
        TWorld: NormVoxelWorld + Eq + std::hash::Hash,
        TWorld::IndexType: num::Integer + std::hash::Hash,
    {
        let (init_world, _) = TWorld::from_voxels([
            (
                [num::zero(); 3].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [num::one(), num::zero(), num::zero()].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, false),
                    false,
                    JointPosition::Minus90,
                ),
            ),
        ])
        .unwrap();
        validate_norm_voxel_world(&init_world);
        let (goal_world, _) = TWorld::from_voxels([
            (
                [num::zero(); 3].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [num::one(), num::zero(), num::zero()].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, false),
                    true,
                    JointPosition::Minus90,
                ),
            ),
        ])
        .unwrap();
        validate_norm_voxel_world(&goal_world);

        let result = compute_reconfig_path(&init_world, goal_world).unwrap();
        assert_eq!(result.len(), 2);
        result
            .iter()
            .map(AsRef::<TWorld>::as_ref)
            .for_each(validate_norm_voxel_world);
    }
}
