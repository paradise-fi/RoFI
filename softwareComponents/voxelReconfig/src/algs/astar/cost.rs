use std::rc::Rc;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Cost<T> {
    /// Cost from init
    pub cost: T,
    /// Potential from the heuristic
    pub potential: T,
}
impl<T> Cost<T> {
    pub fn new(cost: T, potential: T) -> Self {
        Self { cost, potential }
    }
    /// Estimated cost from `init` to `goal`
    pub fn estimated_cost(&self) -> T
    where
        T: std::ops::Add<T, Output = T> + Copy,
    {
        self.cost + self.potential
    }
}

pub struct CostHolder<TState, TCost> {
    pub estimated_cost: TCost,
    pub state: Rc<TState>,
}
impl<TState, TCost> CostHolder<TState, TCost> {
    pub fn new(state: Rc<TState>, estimated_cost: TCost) -> Self {
        Self {
            estimated_cost,
            state,
        }
    }
}

impl<TState, TCost: PartialEq> PartialEq for CostHolder<TState, TCost> {
    fn eq(&self, other: &Self) -> bool {
        self.estimated_cost.eq(&other.estimated_cost)
    }
}
impl<TState, TCost: Eq> Eq for CostHolder<TState, TCost> {}
impl<TState, TCost: PartialOrd> PartialOrd for CostHolder<TState, TCost> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        self.estimated_cost.partial_cmp(&other.estimated_cost)
    }
}
impl<TState, TCost: Ord> Ord for CostHolder<TState, TCost> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.estimated_cost.cmp(&other.estimated_cost)
    }
}
