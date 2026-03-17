//! One directional path finding using traits to accomplish generality.

use super::common::{
    compute_next_step, initialize_parent_map, reconstruct_path_to, NextStepResult,
};
use super::{AlgInfo, Error, ParentMap, StateGraph};
use smallvec::SmallVec;
use std::rc::Rc;

pub fn compute_path<TGraph: StateGraph, TAlgInfo: AlgInfo<TGraph>>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Result<Vec<Rc<TGraph::StateType>>, Error> {
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    if !TGraph::init_check(init, goal) {
        return Err(Error::InitCheckError);
    }

    let parent_map = compute_parents::<TGraph, TAlgInfo>(init, goal).ok_or(Error::PathNotFound)?;

    let goal = parent_map
        .get_key_value(goal)
        .expect("Parent map has to contain goal")
        .0
        .clone();

    Ok(reconstruct_path_to(goal, parent_map, |p| p.0))
}

fn compute_parents<TGraph: StateGraph, TAlgInfo: AlgInfo<TGraph>>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Option<ParentMap<Rc<TGraph::StateType>, TAlgInfo::NodeInfo>> {
    let (init, mut parent_map) = initialize_parent_map::<TGraph, TAlgInfo::NodeInfo>(init);

    let goal_states = TGraph::equivalent_states(goal).collect::<SmallVec<[_; 24]>>();
    if goal_states.contains(&init) {
        return Some(parent_map);
    }

    let mut alg_info = TAlgInfo::new(init, goal);

    loop {
        match compute_next_step(&mut alg_info, &mut parent_map, |s| goal_states.contains(s)) {
            NextStepResult::Continue => {}
            NextStepResult::GoalFound(_) => return Some(parent_map),
            NextStepResult::NoNextState => return None,
        }
    }
}
