use std::collections::HashMap;
use std::hash::BuildHasher;

pub mod astar;
pub mod bfs;

#[derive(Debug, Clone, Copy, amplify::Display, amplify::Error)]
#[display(doc_comments)]
pub enum Error {
    /// Init check failed - no path can exists
    InitCheckError,
    /// Path was not found
    PathNotFound,
}

pub trait StateGraph {
    type StateType: Eq + std::hash::Hash;

    type EqStatesIter<'a>: Iterator<Item = Self::StateType>
    where
        Self::StateType: 'a;
    type NextStatesIter<'a>: Iterator<Item = Self::StateType>
    where
        Self::StateType: 'a;

    fn debug_check_state(state: &Self::StateType);
    fn init_check(init: &Self::StateType, goal: &Self::StateType) -> bool;

    fn equivalent_states(state: &Self::StateType) -> Self::EqStatesIter<'_>;
    fn next_states(state: &Self::StateType) -> Self::NextStatesIter<'_>;
}

fn reconstruct_path_to<TItem: Eq + std::hash::Hash, TParentInfo, S: BuildHasher>(
    goal: TItem,
    parent_map: &HashMap<TItem, TParentInfo, S>,
    mut get_parent: impl FnMut(&TParentInfo) -> Option<TItem>,
) -> Vec<TItem> {
    assert!(parent_map.contains_key(&goal), "Missing goal parent");
    let mut parent = Some(goal);
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
