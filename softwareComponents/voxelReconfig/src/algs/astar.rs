//! Compute any path using the [A* search algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) with early return.

use super::{AlgInfo, ParentMap, StateGraph};
use crate::reconfig::metric::cost::{Cost, CostHolder};
use crate::reconfig::metric::Metric;
use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::rc::Rc;

pub struct AstarAlgInfo<TState, TMetric: Metric<TState>, const OPTIMAL: bool = false> {
    metric: TMetric,
    states_to_visit: BinaryHeap<Reverse<CostHolder<Rc<TState>, TMetric::EstimatedCost>>>,
}

impl<TGraph, TMetric, const OPTIMAL: bool> AlgInfo<TGraph>
    for AstarAlgInfo<TGraph::StateType, TMetric, OPTIMAL>
where
    TGraph: StateGraph,
    TMetric: Metric<TGraph::StateType>,
{
    type NodeInfo = Cost<TMetric::Potential>;
    const EARLY_CHECK: bool = !OPTIMAL;

    fn new(init: Rc<TGraph::StateType>, goal: &TGraph::StateType) -> Self {
        let mut metric = TMetric::new(goal);
        debug_assert!(metric.get_potential(init.as_ref()) >= metric.get_potential(goal));
        Self {
            metric,
            states_to_visit: BinaryHeap::from([Reverse(CostHolder {
                estimated_cost: TMetric::EstimatedCost::default(),
                state: init,
            })]),
        }
    }

    fn visit_next_state(
        &mut self,
        parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<(Rc<TGraph::StateType>, Self::NodeInfo)> {
        let Some(Reverse(CostHolder {
            estimated_cost: current_estimated_cost,
            state: current,
        })) = self.states_to_visit.pop()
        else {
            return None;
        };
        TGraph::debug_check_state(&current);

        let &(_, cost) = parent_map
            .get(&current)
            .expect("Visiting state not in parent map");

        {
            let estimated_cost = TMetric::estimated_cost(cost);
            if current_estimated_cost > estimated_cost {
                // There was a shorter way
                return <Self as AlgInfo<TGraph>>::visit_next_state(self, parent_map);
            }
            // This is the shortest way
            debug_assert_eq!(
                current_estimated_cost, estimated_cost,
                "The lower estimated cost should have been taken first"
            );
        }

        Some((current, cost))
    }

    fn get_node_info(
        &mut self,
        parent_cost: &Self::NodeInfo,
        new_state: &TGraph::StateType,
        parent_map: &ParentMap<Rc<TGraph::StateType>, Self::NodeInfo>,
    ) -> Option<Self::NodeInfo> {
        let real_cost = parent_cost.real_cost + 1;

        if let Some((key, &(_, old_cost))) = parent_map.get_key_value(new_state) {
            TGraph::debug_check_state(&key);

            if real_cost >= old_cost.real_cost {
                debug_assert!(TGraph::equivalent_states(&new_state).all(|eq_state| {
                    parent_map
                        .get(&eq_state)
                        .is_some_and(|(_, c)| c.real_cost == old_cost.real_cost)
                }));
                return Default::default();
            }

            Some(Cost::new(real_cost, old_cost.potential))
        } else {
            Some(Cost::new(real_cost, self.metric.get_potential(&new_state)))
        }
    }

    fn add_state_to_visit(&mut self, state: Rc<TGraph::StateType>, node_info: Self::NodeInfo) {
        self.states_to_visit.push(Reverse(CostHolder::new(
            state,
            TMetric::estimated_cost(node_info),
        )));
    }
}
