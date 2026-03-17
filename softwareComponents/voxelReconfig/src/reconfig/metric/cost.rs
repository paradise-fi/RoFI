#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub struct Cost<TPotential> {
    /// Cost from init
    pub real_cost: usize,
    /// Potential from the metric
    pub potential: TPotential,
}
impl<TPotential> Cost<TPotential> {
    pub fn new(real_cost: usize, potential: TPotential) -> Self {
        Self {
            real_cost,
            potential,
        }
    }
}

#[derive(educe::Educe)]
#[educe(
    PartialEq(bound = "TCost: PartialEq"),
    Eq(bound = "TCost: Eq"),
    PartialOrd(bound = "TCost: PartialOrd"),
    Ord(bound = "TCost: Ord"),
    Hash(bound = "TCost: std::hash::Hash")
)]
pub struct CostHolder<TState, TCost> {
    pub estimated_cost: TCost,
    #[educe(
        PartialEq(ignore),
        Eq(ignore),
        PartialOrd(ignore),
        Ord(ignore),
        Hash(ignore)
    )]
    pub state: TState,
}
impl<TState, TCost> CostHolder<TState, TCost> {
    pub fn new(state: TState, estimated_cost: TCost) -> Self {
        Self {
            estimated_cost,
            state,
        }
    }
}
