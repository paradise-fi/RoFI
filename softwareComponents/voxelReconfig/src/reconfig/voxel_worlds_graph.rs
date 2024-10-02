use super::all_next_worlds_norm;
use crate::algs::StateGraph;
use crate::voxel_world::{check_voxel_world, is_normalized, normalized_eq_worlds, NormVoxelWorld};
use std::assert_matches::{assert_matches, debug_assert_matches};
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
    type EqStatesIter<'a> = impl 'a + Iterator<Item = Self::StateType>
    where
        Self::StateType: 'a;
    type NextStatesIter<'a> = impl 'a + Iterator<Item = Self::StateType>
    where
        Self::StateType: 'a;

    fn debug_check_state(state: &Self::StateType) {
        debug_assert_matches!(check_voxel_world(state), Ok(()));
        debug_assert!(is_normalized(state));
    }
    fn init_check(init: &Self::StateType, goal: &Self::StateType) -> bool {
        assert!(is_normalized(init));
        assert!(is_normalized(goal));
        assert_matches!(check_voxel_world(init), Ok(()));
        assert_matches!(check_voxel_world(goal), Ok(()));

        init.all_voxels().count() == goal.all_voxels().count()
    }

    fn equivalent_states(state: &Self::StateType) -> Self::EqStatesIter<'_> {
        normalized_eq_worlds(state)
    }
    fn next_states(state: &Self::StateType) -> Self::NextStatesIter<'_> {
        all_next_worlds_norm(state)
    }
}
