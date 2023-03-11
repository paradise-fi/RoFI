//! Compute a shortest path using the [breadth-first search algorithm](https://en.wikipedia.org/wiki/Breadth-first_search).

use super::{reconstruct_path_to, Error, StateGraph};
use crate::counters::Counter;
use rustc_hash::FxHashMap;
use std::collections::VecDeque;
use std::rc::Rc;

type ParentMap<TItem> = FxHashMap<Rc<TItem>, Option<Rc<TItem>>>;

pub fn compute_path<TGraph: StateGraph>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Result<Vec<Rc<TGraph::StateType>>, Error> {
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    if !TGraph::init_check(init, goal) {
        return Err(Error::InitCheckError);
    }

    let parent_map = compute_parents::<TGraph>(init, goal).ok_or(Error::PathNotFound)?;

    let goal = parent_map
        .get_key_value(goal)
        .expect("Parent map has to contain goal")
        .0
        .clone();

    Ok(reconstruct_path_to(goal, &parent_map, Clone::clone))
}

fn compute_parents<TGraph: StateGraph>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Option<ParentMap<TGraph::StateType>> {
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    let mut init_states = TGraph::equivalent_states(init).map(Rc::new).peekable();
    let init = init_states.peek().expect("No equivalent state").clone();
    let mut parent_map = init_states
        .map(|init| (init, None))
        .collect::<ParentMap<_>>();
    let mut states_to_visit = VecDeque::from([init]);

    if parent_map.contains_key(goal) {
        return Some(parent_map);
    }

    while let Some(current) = states_to_visit.pop_front() {
        TGraph::debug_check_state(&current);
        for new_state in TGraph::next_states(&current) {
            TGraph::debug_check_state(&new_state);

            if parent_map.contains_key(&new_state) {
                debug_assert!(
                    TGraph::equivalent_states(&new_state)
                        .all(|eq_state| parent_map.contains_key(&eq_state)),
                    "Parent map contains a state, but not all of its eq variants"
                );
                continue;
            }

            debug_assert!(
                TGraph::equivalent_states(&new_state)
                    .all(|eq_state| !parent_map.contains_key(&eq_state)),
                "Parent map contains eq variant of a state but not the state itself"
            );

            let mut eq_states = TGraph::equivalent_states(&new_state)
                .map(Rc::new)
                .peekable();
            let new_state = eq_states.peek().expect("No equivalent state").clone();
            TGraph::debug_check_state(&new_state);

            Counter::saved_new_unique_state();
            parent_map.extend(eq_states.map(|eq_state| (eq_state, Some(current.clone()))));

            if parent_map.contains_key(goal) {
                return Some(parent_map);
            }

            states_to_visit.push_back(new_state);
        }
    }
    None
}
