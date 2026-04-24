use super::all_next_worlds_norm;
use crate::algs::{BoxStateIter, StateGraph};
use crate::voxel_world::{check_voxel_world, is_normalized, normalized_eq_worlds, NormVoxelWorld};
use std::marker::PhantomData;

pub struct VoxelWorldsGraph<TWorld>(PhantomData<TWorld>)
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash;

impl<TWorld> StateGraph for VoxelWorldsGraph<TWorld>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    type StateType = TWorld;
    type EqStatesIter<'a> = BoxStateIter<'a, Self::StateType>
    where
        Self::StateType: 'a;
    type NextStatesIter<'a> = BoxStateIter<'a, Self::StateType>
    where
        Self::StateType: 'a;

    fn debug_check_state(state: &Self::StateType) {
        debug_assert!(matches!(check_voxel_world(state), Ok(())));
        debug_assert!(is_normalized(state));
    }
    fn init_check(init: &Self::StateType, goal: &Self::StateType) -> bool {
        assert!(is_normalized(init));
        assert!(is_normalized(goal));
        assert!(matches!(check_voxel_world(init), Ok(())));
        assert!(matches!(check_voxel_world(goal), Ok(())));

        init.all_voxels().count() == goal.all_voxels().count()
    }

    fn equivalent_states(state: &Self::StateType) -> Self::EqStatesIter<'_> {
        Box::new(normalized_eq_worlds(state))
    }
    fn next_states(state: &Self::StateType) -> Self::NextStatesIter<'_> {
        Box::new(all_next_worlds_norm(state))
    }
}
