//! Compute any path using the [A* search algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) with early return.

pub mod opt;

use crate::algs::{reconstruct_path_to, Error, StateGraph};
use crate::counters::Counter;
use crate::reconfig::metric::cost::{Cost, CostHolder};
use crate::reconfig::metric::Metric;
use rustc_hash::FxHashMap;
use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::rc::Rc;

type ParentMap<TItem, TCost> = FxHashMap<Rc<TItem>, (Option<Rc<TItem>>, Cost<TCost>)>;

/// metric will be called only once on each equivalent state
pub fn compute_path<TGraph, TMetric>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
) -> Result<Vec<Rc<TGraph::StateType>>, Error>
where
    TGraph: StateGraph,
    TMetric: Metric<TGraph::StateType>,
{
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    if !TGraph::init_check(init, goal) {
        return Err(Error::InitCheckError);
    }

    let mut metric = TMetric::new(goal);
    debug_assert!(metric.get_potential(init) >= metric.get_potential(goal));

    let parent_map = compute_parents::<TGraph, _>(init, goal, metric).ok_or(Error::PathNotFound)?;

    let goal = parent_map
        .get_key_value(goal)
        .expect("Parent map has to contain goal")
        .0
        .clone();

    Ok(reconstruct_path_to(goal, parent_map, |p| p.0))
}

fn compute_parents<TGraph, TMetric>(
    init: &TGraph::StateType,
    goal: &TGraph::StateType,
    mut metric: TMetric,
) -> Option<ParentMap<TGraph::StateType, TMetric::Potential>>
where
    TGraph: StateGraph,
    TMetric: Metric<TGraph::StateType>,
{
    TGraph::debug_check_state(init);
    TGraph::debug_check_state(goal);

    let mut init_states = TGraph::equivalent_states(init).map(Rc::new).peekable();
    let init = init_states.peek().expect("No equivalent state").clone();
    let mut parent_map = init_states
        .map(|init| (init, (None, Cost::new(0, TMetric::Potential::default()))))
        .collect::<ParentMap<_, _>>();
    let mut states_to_visit = BinaryHeap::from([Reverse(CostHolder {
        estimated_cost: TMetric::EstimatedCost::default(),
        state: init,
    })]);

    while let Some(Reverse(CostHolder {
        estimated_cost,
        state: current,
    })) = states_to_visit.pop()
    {
        TGraph::debug_check_state(&current);

        let &(_, cost) = parent_map.get(&current).unwrap();

        {
            let old_estimated_cost = TMetric::estimated_cost(cost);
            if estimated_cost > old_estimated_cost {
                // There was a shorter way
                continue;
            }
            assert_eq!(
                estimated_cost, old_estimated_cost,
                "The old estimated cost should be taken first"
            );
        }

        for new_state in TGraph::next_states(&current) {
            TGraph::debug_check_state(&new_state);

            let new_real_cost = cost.real_cost + 1;

            let new_state_rc;
            let new_cost;
            if let Some((key, &(_, old_cost))) = parent_map.get_key_value(&new_state) {
                if new_real_cost >= old_cost.real_cost {
                    debug_assert!(TGraph::equivalent_states(&new_state).all(|eq_state| {
                        parent_map
                            .get(&eq_state)
                            .is_some_and(|(_, c)| c.real_cost == old_cost.real_cost)
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
                new_cost = Cost::new(new_real_cost, metric.get_potential(&new_state_rc));

                debug_assert!(TGraph::equivalent_states(&new_state_rc)
                    .all(|eq_state| !parent_map.contains_key(&eq_state)));

                Counter::saved_new_unique_state();
                parent_map.extend(
                    eq_states.map(|eq_state| (eq_state, (Some(current.clone()), new_cost))),
                );

                if parent_map.contains_key(goal) {
                    return Some(parent_map);
                }
            }
            TGraph::debug_check_state(&new_state_rc);

            states_to_visit.push(Reverse(CostHolder::new(
                new_state_rc,
                TMetric::estimated_cost(new_cost),
            )));
        }
    }
    None
}
