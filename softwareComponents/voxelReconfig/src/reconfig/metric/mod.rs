pub mod cost;
pub mod naive;

use self::cost::Cost;

pub trait Metric<TState> {
    type Potential: std::cmp::Ord + Default + Copy + std::fmt::Debug = usize;
    type EstimatedCost: std::cmp::Ord + Default + std::fmt::Debug = Self::Potential;

    fn new(goal: &TState) -> Self
    where
        Self: Sized;

    fn get_potential(&mut self, state: &TState) -> Self::Potential;

    fn estimated_cost(cost: Cost<Self::Potential>) -> Self::EstimatedCost;
}

pub struct ZeroMetric;
impl<TState> Metric<TState> for ZeroMetric {
    type Potential = ();
    type EstimatedCost = usize;

    fn new(_goal: &TState) -> Self
    where
        Self: Sized,
    {
        Self
    }
    fn get_potential(&mut self, _state: &TState) -> Self::Potential {}
    fn estimated_cost(cost: Cost<Self::Potential>) -> Self::EstimatedCost {
        cost.real_cost
    }
}
