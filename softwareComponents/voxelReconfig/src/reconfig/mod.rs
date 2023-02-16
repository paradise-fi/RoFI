#[cfg(test)]
mod test;

pub mod algs;
pub mod heuristic;

use crate::connectivity::ConnectivityGraph;
use crate::counters::Counter;
use crate::module_move::Move;
use crate::module_repr::is_module_repr;
use crate::module_repr::{get_all_module_reprs, get_other_body};
use crate::voxel_world::as_one_of_norm_eq_world;
use crate::voxel_world::NormVoxelWorld;
use rustc_hash::FxHasher;
use std::collections::HashMap;
use std::hash::{BuildHasher, BuildHasherDefault};
use std::rc::Rc;

pub type FxHashMap<K, V, S = BuildHasherDefault<FxHasher>> = HashMap<K, V, S>;

#[derive(Debug, Clone, Copy, amplify::Display, amplify::Error)]
#[display(doc_comments)]
pub enum Error {
    /// Voxel count doesn't match
    VoxelCountDoesNotMatch,
    /// Reconfiguration path not found
    PathNotFound,
}

fn get_path_to<TWorld: Eq + std::hash::Hash, TParentInfo, S: BuildHasher>(
    goal: &TWorld,
    parent_map: &FxHashMap<Rc<TWorld>, TParentInfo, S>,
    mut get_parent: impl FnMut(&TParentInfo) -> Option<Rc<TWorld>>,
) -> Vec<Rc<TWorld>> {
    let mut parent = Some(
        parent_map
            .get_key_value(goal)
            .expect("Missing goal parent")
            .0
            .clone(),
    );
    let mut path = std::iter::from_fn(|| {
        let new_parent = parent_map
            .get(parent.as_ref()?)
            .map(&mut get_parent)
            .expect("Missing prev node");
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
    Counter::new_successors_call();
    let graph = Rc::new(ConnectivityGraph::compute_from(world));
    get_all_module_reprs(world)
        .map(move |module| (module, graph.clone()))
        .flat_map(move |(module, graph)| {
            assert!(is_module_repr(module.1));
            Counter::new_module();
            let other_body = get_other_body(module, world).unwrap();
            graph
                .all_cuts_by_module(module)
                .map(Rc::new)
                .flat_map(move |split| {
                    Move::all_possible_moves(module.1, other_body.1).flat_map(move |module_move| {
                        Counter::new_move();
                        let result = module_move.apply(module, split.clone().as_ref());
                        if result.is_none() {
                            Counter::move_collided();
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
