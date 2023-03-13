use super::{Connections, VoxelWorldWithConnections};
use crate::reconfig::heuristic::Heuristic;
use crate::voxel_world::NormVoxelWorld;

pub fn use_world_heuristic<'a, TWorld, TConnections>(
    heuristic: Heuristic,
    goal: &VoxelWorldWithConnections<TWorld, TConnections>,
) -> impl 'a + FnMut(&VoxelWorldWithConnections<TWorld, TConnections>) -> usize
where
    TWorld: 'a + NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
    TConnections: Connections<IndexType = TWorld::IndexType>,
{
    let mut heuristic = heuristic.get_fn(goal.world.as_ref());
    move |world| heuristic(&world.world)
}
