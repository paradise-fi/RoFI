#[cfg(test)]
mod test;

pub mod metric;
pub mod voxel_worlds_graph;

use crate::connectivity::ConnectivityGraph;
use crate::counters::Counter;
use crate::module_move::Move;
use crate::module_repr::is_module_repr;
use crate::module_repr::{get_all_module_reprs, get_other_body};
use crate::voxel_world::as_one_of_norm_eq_world;
use crate::voxel_world::NormVoxelWorld;
use std::rc::Rc;

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
            graph.all_cuts_by_module(module).flat_map(move |split| {
                Move::all_possible_moves(module.1, other_body.1).filter_map(move |module_move| {
                    Counter::new_move();
                    let result = module_move.apply(module, &split);
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
