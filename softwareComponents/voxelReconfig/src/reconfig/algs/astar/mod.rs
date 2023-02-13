//! Compute a shortest path (or all shorted paths) using the [A* search algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm).

#[cfg(test)]
mod test;

pub mod nopt;

pub use crate::reconfig::heuristic::Heuristic;

use crate::reconfig::{all_next_worlds_norm, get_path_to, Error, FxHashMap};
use crate::reconfig::{log_counters, ADDED_WORLDS, NEXT_WORLDS_BEFORE_ADD, STEPS_COMPUTED};
use crate::voxel_world::{as_one_of_norm_eq_world, is_normalized, normalized_eq_worlds};
use crate::voxel_world::{check_voxel_world, NormVoxelWorld};
use std::assert_matches::{assert_matches, debug_assert_matches};
use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::rc::Rc;
use std::sync::atomic;

type ParentMap<TWorld, TCost> = FxHashMap<Rc<TWorld>, (Option<Rc<TWorld>>, Cost<TCost>)>;

/// `heuristic` will be called only once on each equivalent world
pub fn compute_reconfig_path<TWorld>(
    init: TWorld,
    goal: TWorld,
    heuristic: Heuristic,
) -> Result<Vec<Rc<TWorld>>, Error>
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
    let is_goal = |world: &TWorld| {
        debug_assert!(world.all_voxels().count() == goal.all_voxels().count());
        normalized_eq_worlds(world).any(|world| world == goal)
    };
    let mut heuristic = heuristic.get_fn(&goal);
    debug_assert!(heuristic(&init) >= heuristic(&goal));

    let parent_map: ParentMap<TWorld, usize> =
        compute_reconfig_parents(&init, is_goal, heuristic).ok_or(Error::PathNotFound)?;

    log_counters();

    Ok(get_path_to(&goal, &parent_map, |parent_info| {
        parent_info.0.clone()
    }))
}

struct CostHolder<TWorld, TCost> {
    estimated_cost: TCost,
    world: Rc<TWorld>,
}
impl<TWorld, TCost> CostHolder<TWorld, TCost>
where
    TCost: std::ops::Add<TCost, Output = TCost>,
{
    pub fn new(world: Rc<TWorld>, estimated_cost: TCost) -> Self {
        Self {
            estimated_cost,
            world,
        }
    }
}

impl<TWorld, TCost: PartialEq> PartialEq for CostHolder<TWorld, TCost> {
    fn eq(&self, other: &Self) -> bool {
        self.estimated_cost.eq(&other.estimated_cost)
    }
}
impl<TWorld, TCost: Eq> Eq for CostHolder<TWorld, TCost> {}
impl<TWorld, TCost: PartialOrd> PartialOrd for CostHolder<TWorld, TCost> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        self.estimated_cost.partial_cmp(&other.estimated_cost)
    }
}
impl<TWorld, TCost: Ord> Ord for CostHolder<TWorld, TCost> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.estimated_cost.cmp(&other.estimated_cost)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct Cost<T> {
    /// Cost from init
    cost: T,
    /// Potential from the heuristic
    potential: T,
}
impl<T> Cost<T> {
    fn new(cost: T, potential: T) -> Self {
        Self { cost, potential }
    }
    /// Estimated cost from `init` to `goal`
    fn estimated_cost(&self) -> T
    where
        T: std::ops::Add<T, Output = T> + Copy,
    {
        self.cost + self.potential
    }
}

fn compute_reconfig_parents<TWorld, TCost>(
    init: &TWorld,
    mut is_goal: impl FnMut(&TWorld) -> bool,
    mut heuristic: impl FnMut(&TWorld) -> TCost,
) -> Option<ParentMap<TWorld, TCost>>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
    TCost: num::Num + Ord + Copy + std::fmt::Debug,
{
    let mut init_worlds = normalized_eq_worlds(init).map(Rc::new).peekable();
    let init = init_worlds.peek().expect("No normalized variant").clone();
    let mut parent_map = init_worlds
        .map(|init| (init, (None, Cost::new(num::zero(), num::zero()))))
        .collect::<ParentMap<_, _>>();
    let mut worlds_to_visit = BinaryHeap::from([Reverse(CostHolder {
        estimated_cost: num::zero(),
        world: init,
    })]);

    while let Some(Reverse(CostHolder {
        estimated_cost,
        world: current,
    })) = worlds_to_visit.pop()
    {
        debug_assert_matches!(check_voxel_world(current.as_ref()), Ok(()));
        debug_assert!(is_normalized(current.as_ref()));

        if is_goal(&current) {
            return Some(parent_map);
        }
        let &(_, cost) = parent_map.get(&current).unwrap();
        if estimated_cost > cost.estimated_cost() {
            // There is a shorter way
            continue;
        }
        debug_assert_eq!(estimated_cost, cost.estimated_cost());

        STEPS_COMPUTED.fetch_add(1, atomic::Ordering::Relaxed);
        for new_world in all_next_worlds_norm(current.as_ref()) {
            NEXT_WORLDS_BEFORE_ADD.fetch_add(1, atomic::Ordering::Relaxed);
            debug_assert_matches!(check_voxel_world(&new_world), Ok(()));
            debug_assert!(is_normalized(&new_world));

            let new_real_cost = cost.cost + num::one();

            let new_world_rc;
            let new_cost;
            if let Some((key, &(_, old_cost))) = parent_map.get_key_value(&new_world) {
                if old_cost.cost <= new_real_cost {
                    debug_assert!(normalized_eq_worlds(&new_world).all(|norm_world| {
                        parent_map
                            .get(&norm_world)
                            .is_some_and(|(_, c)| c.cost == old_cost.cost)
                    }));
                    continue;
                }
                new_world_rc = key.clone();
                new_cost = Cost::new(new_real_cost, old_cost.potential);
                for norm_world in normalized_eq_worlds(&new_world) {
                    let (norm_old_parent, norm_old_cost) = parent_map.get_mut(&norm_world).unwrap();
                    debug_assert!(norm_old_parent.is_some(), "Cannot get better path to init");
                    debug_assert_eq!(norm_old_cost, &old_cost);
                    *norm_old_parent = Some(current.clone());
                    *norm_old_cost = new_cost;
                }
            } else {
                let mut norm_worlds = normalized_eq_worlds(&new_world).map(Rc::new).peekable();
                new_world_rc = norm_worlds.peek().expect("No normalized variant").clone();
                new_cost = Cost::new(
                    new_real_cost,
                    heuristic(&new_world_rc), // num::zero(),
                );

                assert!(normalized_eq_worlds(new_world_rc.as_ref())
                    .all(|norm_world| !parent_map.contains_key(&norm_world)));

                parent_map.extend(
                    norm_worlds.map(|norm_world| (norm_world, (Some(current.clone()), new_cost))),
                );
            }
            debug_assert!(is_normalized(new_world_rc.as_ref()));
            worlds_to_visit.push(Reverse(CostHolder::new(
                new_world_rc,
                new_cost.estimated_cost(),
            )));
            ADDED_WORLDS.fetch_add(1, atomic::Ordering::Relaxed);
        }
    }
    None
}
