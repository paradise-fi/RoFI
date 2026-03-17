//! Compute a shortest path using the [breadth-first search algorithm](https://en.wikipedia.org/wiki/Breadth-first_search).

use super::{AlgInfo, ParentMap, StateGraph};
use std::collections::VecDeque;
use std::rc::Rc;

pub struct BfsAlgInfo<TState> {
    states_to_visit: VecDeque<Rc<TState>>,
}

impl<TGraph: StateGraph> AlgInfo<TGraph> for BfsAlgInfo<TGraph::StateType> {
    type NodeInfo = ();
    const EARLY_CHECK: bool = true;

    fn new(init: Rc<TGraph::StateType>, _goal: &TGraph::StateType) -> Self {
        Self {
            states_to_visit: VecDeque::from([init]),
        }
    }

    fn visit_next_state(
        &mut self,
        _parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<(Rc<TGraph::StateType>, Self::NodeInfo)> {
        self.states_to_visit.pop_front().map(|s| (s, ()))
    }

    fn get_node_info(
        &mut self,
        _parent_node_info: &Self::NodeInfo,
        new_state: &TGraph::StateType,
        parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<Self::NodeInfo> {
        if parent_map.contains_key(new_state) {
            debug_assert!(
                TGraph::equivalent_states(new_state)
                    .all(|eq_state| parent_map.contains_key(&eq_state)),
                "Parent map contains a state, but not all of its eq variants"
            );
            return None;
        }

        debug_assert!(
            TGraph::equivalent_states(new_state)
                .all(|eq_state| !parent_map.contains_key(&eq_state)),
            "Parent map contains eq variant of a state but not the state itself"
        );
        Some(())
    }

    fn add_state_to_visit(&mut self, state: Rc<TGraph::StateType>, _node_info: Self::NodeInfo) {
        self.states_to_visit.push_back(state);
    }
}
