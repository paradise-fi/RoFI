pub mod assignment;
pub mod cost;
pub mod naive;

use self::cost::Cost;
use crate::pos::Pos;
use crate::voxel_world::{CenteredVoxelWorld, VoxelWorld};

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

fn get_median_pos<TIndex: num::Num + Ord + Copy>(mut positions: Vec<Pos<TIndex>>) -> Pos<TIndex> {
    positions.sort_by_key(|pos| pos.x);
    let x_median = positions[positions.len() / 2].x;
    positions.sort_by_key(|pos| pos.y);
    let y_median = positions[positions.len() / 2].y;
    positions.sort_by_key(|pos| pos.z);
    let z_median = positions[positions.len() / 2].z;

    Pos::from([x_median, y_median, z_median])
}

fn centered_by_pos_median<TWorld, TWorldRef>(
    world: TWorldRef,
) -> CenteredVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    let pos_median = get_median_pos(world.borrow().all_voxels().map(|(pos, _)| pos).collect());
    CenteredVoxelWorld::new(world, pos_median)
}
