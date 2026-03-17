use super::Metric;
use num::integer::Roots;
use num::traits::real::Real;
use num::FromPrimitive;
use std::marker::PhantomData;

pub trait PotentialFn<TState> {
    type Potential: std::cmp::Ord + Default + Copy + std::fmt::Debug = usize;

    fn new(goal: &TState) -> Self
    where
        Self: Sized;

    fn get_potential(&mut self, state: &TState) -> Self::Potential;
}

// Uses the `potential + real_cost` for the estimated cost
pub struct Sum<TState, TPotentialFn>(pub TPotentialFn, PhantomData<TState>)
where
    TPotentialFn: PotentialFn<TState>;

impl<TState, TPotentialFn> Metric<TState> for Sum<TState, TPotentialFn>
where
    TPotentialFn: PotentialFn<TState>,
    TPotentialFn::Potential: FromPrimitive + num::Num,
{
    type Potential = TPotentialFn::Potential;
    type EstimatedCost = Self::Potential;

    fn new(goal: &TState) -> Self {
        Self(PotentialFn::new(goal), Default::default())
    }

    fn get_potential(&mut self, state: &TState) -> Self::Potential {
        self.0.get_potential(state)
    }

    fn estimated_cost(cost: super::cost::Cost<Self::Potential>) -> Self::EstimatedCost {
        cost.potential
            + FromPrimitive::from_usize(cost.real_cost).expect("Cannot convert real cost")
    }
}

// Uses the `potential + integer_sqrt(real_cost)` for the estimated cost
pub struct ISqrtSum<TState, TPotentialFn>(pub TPotentialFn, PhantomData<TState>)
where
    TPotentialFn: PotentialFn<TState>;

impl<TState, TPotentialFn> Metric<TState> for ISqrtSum<TState, TPotentialFn>
where
    TPotentialFn: PotentialFn<TState>,
    TPotentialFn::Potential: num::Integer + FromPrimitive,
{
    type Potential = TPotentialFn::Potential;
    type EstimatedCost = TPotentialFn::Potential;

    fn new(goal: &TState) -> Self {
        Self(PotentialFn::new(goal), Default::default())
    }

    fn get_potential(&mut self, state: &TState) -> Self::Potential {
        self.0.get_potential(state)
    }

    fn estimated_cost(cost: super::cost::Cost<Self::Potential>) -> Self::EstimatedCost {
        cost.potential
            + FromPrimitive::from_usize(cost.real_cost.sqrt()).expect("Cannot convert real cost")
    }
}

// Uses the `potential + sqrt(real_cost)` for the estimated cost
pub struct FSqrtSum<TState, TPotentialFn>(pub TPotentialFn, PhantomData<TState>)
where
    TPotentialFn: PotentialFn<TState>;

impl<TState, TPotentialFn> Metric<TState> for FSqrtSum<TState, TPotentialFn>
where
    TPotentialFn: PotentialFn<TState>,
    TPotentialFn::Potential: FromPrimitive + Real,
{
    type Potential = TPotentialFn::Potential;
    type EstimatedCost = TPotentialFn::Potential;

    fn new(goal: &TState) -> Self {
        Self(PotentialFn::new(goal), Default::default())
    }

    fn get_potential(&mut self, state: &TState) -> Self::Potential {
        self.0.get_potential(state)
    }

    fn estimated_cost(cost: super::cost::Cost<Self::Potential>) -> Self::EstimatedCost {
        let real_cost =
            Self::EstimatedCost::from_usize(cost.real_cost).expect("Cannot convert real cost");
        cost.potential + real_cost.sqrt()
    }
}
