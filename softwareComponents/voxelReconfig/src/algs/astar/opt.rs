//! Compute a shortest path (or all shorted paths) using the [A* search algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm).

pub use crate::reconfig::heuristic::Heuristic;

use super::cost::{Cost, CostHolder};
use crate::algs::{reconstruct_path_to, Error, StateGraph};
use crate::counters::Counter;
use rustc_hash::FxHashMap;
use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::rc::Rc;

type ParentMap<TItem, TCost> = FxHashMap<Rc<TItem>, (Option<Rc<TItem>>, Cost<TCost>)>;

/// `heuristic` will be called only once on each equivalent state
pub fn compute_path<TGraph, TCost>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
    mut heuristic: impl FnMut(&TGraph::StateType) -> TCost,
) -> Result<Vec<Rc<TGraph::StateType>>, Error>
where
    TGraph: StateGraph,
    TCost: num::Num + Ord + Copy + std::fmt::Debug,
{
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    if !TGraph::init_check(init, goal) {
        return Err(Error::InitCheckError);
    }

    debug_assert!(heuristic(init) >= heuristic(goal));

    let is_goal =
        |state: &TGraph::StateType| TGraph::equivalent_states(state).any(|state| &state == goal);

    let parent_map =
        compute_parents::<TGraph, _>(init, is_goal, heuristic).ok_or(Error::PathNotFound)?;

    let goal = parent_map
        .get_key_value(goal)
        .expect("Parent map has to contain goal")
        .0
        .clone();

    Ok(reconstruct_path_to(goal, &parent_map, |parent_info| {
        parent_info.0.clone()
    }))
}

fn compute_parents<TGraph, TCost>(
    init: &TGraph::StateType,
    is_goal: impl Fn(&TGraph::StateType) -> bool,
    mut heuristic: impl FnMut(&TGraph::StateType) -> TCost,
) -> Option<ParentMap<TGraph::StateType, TCost>>
where
    TGraph: StateGraph,
    TCost: num::Num + Ord + Copy + std::fmt::Debug,
{
    TGraph::debug_check_state(init);

    let mut init_states = TGraph::equivalent_states(init).map(Rc::new).peekable();
    let init = init_states.peek().expect("No equivalent state").clone();
    let mut parent_map = init_states
        .map(|init| (init, (None, Cost::new(num::zero(), num::zero()))))
        .collect::<ParentMap<_, _>>();
    let mut states_to_visit = BinaryHeap::from([Reverse(CostHolder {
        estimated_cost: num::zero(),
        state: init,
    })]);

    while let Some(Reverse(CostHolder {
        estimated_cost,
        state: current,
    })) = states_to_visit.pop()
    {
        TGraph::debug_check_state(&current);

        if is_goal(&current) {
            return Some(parent_map);
        }
        let &(_, cost) = parent_map.get(&current).unwrap();
        if estimated_cost > cost.estimated_cost() {
            // There was a shorter way
            continue;
        }
        assert_eq!(estimated_cost, cost.estimated_cost());

        for new_state in TGraph::next_states(&current) {
            TGraph::debug_check_state(&new_state);

            let new_real_cost = cost.cost + num::one();

            let new_state_rc;
            let new_cost;
            if let Some((key, &(_, old_cost))) = parent_map.get_key_value(&new_state) {
                if new_real_cost >= old_cost.cost {
                    debug_assert!(TGraph::equivalent_states(&new_state).all(|eq_state| {
                        parent_map
                            .get(&eq_state)
                            .is_some_and(|(_, c)| c.cost == old_cost.cost)
                    }));
                    continue;
                }
                new_state_rc = key.clone();
                new_cost = Cost::new(new_real_cost, old_cost.potential);
                for eq_state in TGraph::equivalent_states(&new_state) {
                    let (eq_old_parent, eq_old_cost) = parent_map
                        .get_mut(&eq_state)
                        .expect("Parent map contains a state, but not all of its eq variants");
                    debug_assert!(eq_old_parent.is_some(), "Cannot get better path to init");
                    debug_assert_eq!(eq_old_cost, &old_cost);
                    *eq_old_parent = Some(current.clone());
                    *eq_old_cost = new_cost;
                }
            } else {
                let mut eq_states = TGraph::equivalent_states(&new_state)
                    .map(Rc::new)
                    .peekable();
                new_state_rc = eq_states.peek().expect("No equivalent state").clone();
                new_cost = Cost::new(new_real_cost, heuristic(&new_state_rc));

                debug_assert!(TGraph::equivalent_states(&new_state_rc)
                    .all(|eq_state| !parent_map.contains_key(&eq_state)));

                Counter::saved_new_unique_state();
                parent_map.extend(
                    eq_states.map(|eq_state| (eq_state, (Some(current.clone()), new_cost))),
                );
            }
            TGraph::debug_check_state(&new_state_rc);

            states_to_visit.push(Reverse(CostHolder::new(
                new_state_rc,
                new_cost.estimated_cost(),
            )));
        }
    }
    None
}
