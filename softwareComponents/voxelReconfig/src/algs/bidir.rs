//! [Bidirectional path finding](https://en.wikipedia.org/wiki/Bidirectional_search) using traits to accomplish generality.

use super::common::{
    compute_next_step, initialize_parent_map, reconstruct_path_to, NextStepResult,
};
use super::{AlgInfo, Error, ParentMap, StateGraph};
use std::rc::Rc;

struct BidirectionalResult<TItem, TNodeInfo> {
    parent_map: ParentMap<TItem, TNodeInfo>,
    rev_parent_map: ParentMap<TItem, TNodeInfo>,
    mid_state: TItem,
}

pub fn compute_path<TGraph: StateGraph, TAlgInfo: AlgInfo<TGraph>>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Result<Vec<Rc<TGraph::StateType>>, Error> {
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    if !TGraph::init_check(init, goal) {
        return Err(Error::InitCheckError);
    }

    let parent_maps = compute_parents::<TGraph, TAlgInfo>(init, goal).ok_or(Error::PathNotFound)?;

    let init_to_mid =
        reconstruct_path_to(parent_maps.mid_state.clone(), parent_maps.parent_map, |p| {
            p.0
        });
    let goal_to_mid =
        reconstruct_path_to(parent_maps.mid_state, parent_maps.rev_parent_map, |p| p.0);

    let mut path = init_to_mid;
    path.extend(goal_to_mid.into_iter().rev().skip(1));
    Ok(path)
}

fn compute_parents<TGraph: StateGraph, TAlgInfo: AlgInfo<TGraph>>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Option<BidirectionalResult<Rc<TGraph::StateType>, TAlgInfo::NodeInfo>> {
    let (init, mut parent_map) = initialize_parent_map::<TGraph, TAlgInfo::NodeInfo>(init);
    let (goal, mut rev_parent_map) = initialize_parent_map::<TGraph, TAlgInfo::NodeInfo>(goal);

    if parent_map.contains_key(&goal) {
        return Some(BidirectionalResult {
            parent_map,
            rev_parent_map,
            mid_state: goal,
        });
    }

    let mut alg_info = TAlgInfo::new(init.clone(), &goal);
    let mut rev_alg_info = TAlgInfo::new(goal, &init);

    loop {
        match compute_next_step(&mut alg_info, &mut parent_map, |s| {
            rev_parent_map.contains_key(s)
        }) {
            NextStepResult::Continue => {}
            NextStepResult::NoNextState => return None,
            NextStepResult::GoalFound(mid_state) => {
                return Some(BidirectionalResult {
                    parent_map,
                    rev_parent_map,
                    mid_state,
                })
            }
        }

        match compute_next_step(&mut rev_alg_info, &mut rev_parent_map, |s| {
            parent_map.contains_key(s)
        }) {
            NextStepResult::Continue => {}
            NextStepResult::NoNextState => return None,
            NextStepResult::GoalFound(mid_state) => {
                return Some(BidirectionalResult {
                    parent_map,
                    rev_parent_map,
                    mid_state,
                })
            }
        }
    }
}
