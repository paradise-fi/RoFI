use crate::voxel_world::NormVoxelWorld;

pub enum Heuristic {
    Zero,
}

impl Heuristic {
    pub fn get_fn<'a, TWorld>(self, goal: &TWorld) -> Box<dyn FnMut(&TWorld) -> usize + 'a>
    where
        TWorld: 'a + NormVoxelWorld + Eq + std::hash::Hash,
        TWorld::IndexType: num::Integer + std::hash::Hash,
    {
        match self {
            Self::Zero => Box::new(|_| 0),
        }
    }
}
