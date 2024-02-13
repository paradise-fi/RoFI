use super::{AlgInfo, ParentMap, StateGraph};
use crate::counters::Counter;
use std::collections::HashMap;
use std::hash::BuildHasher;
use std::rc::Rc;

pub fn initialize_parent_map<TGraph: StateGraph, TNodeInfo: Default>(
    init: &TGraph::StateType,
) -> (
    Rc<TGraph::StateType>,
    ParentMap<Rc<TGraph::StateType>, TNodeInfo>,
) {
    let mut init_states = TGraph::equivalent_states(init).map(Rc::new).peekable();
    let init = init_states.peek().expect("No equivalent state").clone();
    let parent_map = init_states.map(|init| (init, Default::default())).collect();
    (init, parent_map)
}

fn add_to_parent_map<TGraph: StateGraph, TNodeInfo: Clone>(
    new_state: &TGraph::StateType,
    new_node_info: &TNodeInfo,
    parent_state: &Rc<TGraph::StateType>,
    parent_map: &mut ParentMap<Rc<TGraph::StateType>, TNodeInfo>,
) -> Rc<TGraph::StateType> {
    let mut eq_states = TGraph::equivalent_states(&new_state)
        .map(Rc::new)
        .peekable();
    let new_state = eq_states.peek().expect("No equivalent state").clone();
    TGraph::debug_check_state(&new_state);

    parent_map.extend(eq_states.map(|eq_state| {
        (
            eq_state,
            (Some(parent_state.clone()), new_node_info.clone()),
        )
    }));

    new_state
}

fn update_parent_map<TGraph: StateGraph, TNodeInfo: Clone>(
    new_state: &TGraph::StateType,
    new_node_info: &TNodeInfo,
    parent_state: &Rc<TGraph::StateType>,
    parent_map: &mut ParentMap<Rc<TGraph::StateType>, TNodeInfo>,
) {
    for eq_state in TGraph::equivalent_states(new_state) {
        let eq_state_parent_info = parent_map
            .get_mut(&eq_state)
            .expect("Parent map contains a state, but not all of its eq variants");
        debug_assert!(
            eq_state_parent_info.0.is_some(),
            "Cannot get better path to init"
        );
        *eq_state_parent_info = (Some(parent_state.clone()), new_node_info.clone());
    }
}

pub enum NextStepResult<TState> {
    Continue,
    GoalFound(TState),
    NoNextState,
}

pub fn compute_next_step<TGraph: StateGraph, TAlgInfo: AlgInfo<TGraph>>(
    alg_info: &mut TAlgInfo,
    parent_map: &mut ParentMap<Rc<TGraph::StateType>, TAlgInfo::NodeInfo>,
    mut is_goal: impl FnMut(&TGraph::StateType) -> bool,
) -> NextStepResult<Rc<TGraph::StateType>> {
    let Some((current, current_node_info)) = alg_info.visit_next_state(&parent_map) else {
        return NextStepResult::NoNextState;
    };
    TGraph::debug_check_state(&current);

    if !TAlgInfo::EARLY_CHECK && is_goal(&current) {
        return NextStepResult::GoalFound(current);
    }

    for new_state in TGraph::next_states(&current) {
        TGraph::debug_check_state(&new_state);

        let Some(new_node_info) =
            alg_info.get_node_info(&current_node_info, &new_state, &parent_map)
        else {
            continue;
        };

        if let Some((new_state, _)) = parent_map.get_key_value(&new_state) {
            let new_state = new_state.clone();
            update_parent_map::<TGraph, _>(&new_state, &new_node_info, &current, parent_map);
            alg_info.add_state_to_visit(new_state, new_node_info);
        } else {
            debug_assert!(TGraph::equivalent_states(&new_state)
                .all(|eq_state| !parent_map.contains_key(&eq_state)));

            Counter::saved_new_unique_state();

            let new_state =
                add_to_parent_map::<TGraph, _>(&new_state, &new_node_info, &current, parent_map);

            if TAlgInfo::EARLY_CHECK && is_goal(&new_state) {
                return NextStepResult::GoalFound(new_state);
            }

            alg_info.add_state_to_visit(new_state, new_node_info);
        }
    }
    NextStepResult::Continue
}

pub fn reconstruct_path_to<TItem: Eq + std::hash::Hash, TParentInfo, S: BuildHasher>(
    goal: TItem,
    mut parent_map: HashMap<TItem, TParentInfo, S>,
    mut get_parent: impl FnMut(TParentInfo) -> Option<TItem>,
) -> Vec<TItem> {
    assert!(parent_map.contains_key(&goal), "Missing goal parent");
    let mut path = Vec::new();
    let mut parent = goal;
    loop {
        if let Some(new_parent_info) = parent_map.remove(&parent) {
            if let Some(new_parent) = get_parent(new_parent_info) {
                path.push(std::mem::replace(&mut parent, new_parent));
            } else {
                path.push(parent);
                break;
            }
        } else {
            if path.contains(&parent) {
                panic!("Cyclic dependency in parent graph");
            } else {
                panic!("Missing parent info");
            }
        }
    }

    path.reverse();
    path
}

#[allow(unused)]
pub fn reconstruct_path_to_noconsume<TItem: Eq + std::hash::Hash, TParentInfo, S: BuildHasher>(
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
