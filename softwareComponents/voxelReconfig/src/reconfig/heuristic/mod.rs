pub mod naive;

use crate::voxel_world::NormVoxelWorld;

pub enum Heuristic {
    Zero,
    Naive,
}

impl Heuristic {
    pub fn get_fn<'a, TWorld>(self, goal: &TWorld) -> Box<dyn 'a + FnMut(&TWorld) -> usize>
    where
        TWorld: 'a + NormVoxelWorld + Eq + std::hash::Hash,
        TWorld::IndexType: num::Integer + std::hash::Hash,
    {
        match self {
            Self::Zero => Box::new(|_| 0),
            Self::Naive => Box::new(naive::NaiveHeuristic::new(goal).get_fn()),
        }
    }
}
