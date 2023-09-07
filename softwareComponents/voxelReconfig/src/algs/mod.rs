mod common;

pub mod astar;
pub mod bfs;

pub mod bidir;
pub mod onedir;

use rustc_hash::FxHashMap;
use std::rc::Rc;

pub type ParentMap<TItem, TNodeInfo> = FxHashMap<TItem, (Option<TItem>, TNodeInfo)>;

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

pub trait AlgInfo<TGraph: StateGraph> {
    type NodeInfo: Default + Clone = ();
    const EARLY_CHECK: bool;

    fn new(init: Rc<TGraph::StateType>, goal: &TGraph::StateType) -> Self
    where
        Self: Sized;

    fn visit_next_state(
        &mut self,
        parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<(Rc<TGraph::StateType>, Self::NodeInfo)>;
    fn get_node_info(
        &mut self,
        parent_node_info: &Self::NodeInfo,
        new_state: &TGraph::StateType,
        parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<Self::NodeInfo>;
    fn add_state_to_visit(&mut self, state: Rc<TGraph::StateType>, node_info: Self::NodeInfo);
}
