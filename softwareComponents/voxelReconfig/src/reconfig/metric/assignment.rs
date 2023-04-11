//! Metric based on best assignment algorithm.
//! This limits the metric to evaluating only a pair of modules.

use super::centered_by_pos_median;
use super::{cost::Cost, Metric};
use crate::module_repr::{get_all_module_reprs, get_other_body};
use crate::pos::Pos;
use crate::voxel::{JointPosition, Voxel};
use crate::voxel_world::normalized_eq_worlds;
use crate::voxel_world::{CenteredVoxelWorld, NormVoxelWorld, VoxelWorld};
use iter_fixed::IntoIteratorFixed;
use ndarray::Array2;
use num::{Float, ToPrimitive};
use ordered_float::OrderedFloat;

fn voxel_joint_diff<N: num::Zero + num::One>(goal: Voxel, other: Voxel) -> N {
    match (goal.joint_pos(), other.joint_pos()) {
        (JointPosition::Zero, JointPosition::Zero) => num::zero(),
        (JointPosition::Zero, _) | (_, JointPosition::Zero) => num::one(),
        _ => num::zero(),
    }
}
pub fn voxel_diff<N: num::Num + PartialOrd>(goal: [Voxel; 2], other: [Voxel; 2]) -> N {
    let [goal_a, goal_b] = goal;
    let [other_a, other_b] = other;

    let goal_gamma_rot = goal_a.shoe_rotated() != goal_b.shoe_rotated();
    let other_gamma_rot = other_a.shoe_rotated() != other_b.shoe_rotated();
    let gamma_different = goal_gamma_rot != other_gamma_rot;
    let gamma_diff = if gamma_different { N::one() } else { N::zero() };

    let goal_same_shoe_rot = match (goal_a.joint_pos(), goal_b.joint_pos()) {
        (JointPosition::Zero, _) | (_, JointPosition::Zero) => None,
        _ => Some(goal_a.joint_pos() == goal_b.joint_pos()),
    };
    let other_same_shoe_rot = match (goal_a.joint_pos(), goal_b.joint_pos()) {
        (JointPosition::Zero, _) | (_, JointPosition::Zero) => None,
        _ => Some(goal_a.joint_pos() == goal_b.joint_pos()),
    };
    let opposite_shoe_rot_penalty = match (goal_same_shoe_rot, other_same_shoe_rot) {
        (None, _) | (_, None) => N::zero(),
        _ if goal_same_shoe_rot == other_same_shoe_rot => N::zero(),
        _ => N::one() + N::one(),
    };

    voxel_joint_diff::<N>(goal_a, other_a)
        + voxel_joint_diff::<N>(goal_b, other_b)
        + opposite_shoe_rot_penalty
        + gamma_diff
}

fn pos_cost<TIndex: num::Signed>(lhs: Pos<TIndex>, rhs: Pos<TIndex>) -> TIndex {
    lhs.as_array()
        .into_iter_fixed()
        .zip(rhs.as_array())
        .map(|(lhs, rhs)| num::abs_sub(lhs, rhs))
        .into_iter()
        .fold(num::zero(), TIndex::add)
}

fn compute_best_mapping_cost<TWorld, TGoalWorld>(other: &TWorld, goal: &TGoalWorld) -> f32
where
    TWorld: VoxelWorld<IndexType = TGoalWorld::IndexType>,
    TGoalWorld: VoxelWorld,
    TWorld::IndexType: ToPrimitive,
{
    // let other_modules = get_world_modules(other);
    // let goal_modules = get_world_modules(goal);
    let other_modules_count = get_all_module_reprs(other).count();
    let goal_modules_count = get_all_module_reprs(goal).count();
    assert_eq!(other_modules_count, goal_modules_count);

    let mut costs = Array2::zeros((other_modules_count, goal_modules_count));
    for (other_body_a, i) in get_all_module_reprs(other).zip(0..) {
        let other_body_b = get_other_body(other_body_a, other).unwrap();
        for (goal_body_a, j) in get_all_module_reprs(goal).zip(0..) {
            let goal_body_b = get_other_body(goal_body_a, goal).unwrap();

            // Comparing shoeA with shoeB will always have equal or higher pos cost
            let pos_cost =
                pos_cost(other_body_a.0, goal_body_a.0) + pos_cost(other_body_b.0, goal_body_b.0);
            let pos_cost = ToPrimitive::to_f32(&pos_cost).unwrap();
            let voxel_cost = f32::min(
                voxel_diff(
                    [goal_body_a.1, goal_body_b.1],
                    [other_body_a.1, other_body_b.1],
                ),
                voxel_diff(
                    [goal_body_b.1, goal_body_a.1],
                    [other_body_a.1, other_body_b.1],
                ),
            );
            costs[(i, j)] = pos_cost + voxel_cost;
        }
    }

    let best_mapping = lapjv::lapjv(&costs).unwrap();
    lapjv::cost(&costs, &best_mapping.0)
}

pub struct AssignmentMetric<TWorld>
where
    TWorld: VoxelWorld,
{
    goals: Vec<CenteredVoxelWorld<TWorld, TWorld>>,
}

impl<TWorld> Metric<TWorld> for AssignmentMetric<TWorld>
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: ToPrimitive,
{
    type Potential = OrderedFloat<f32>;
    type EstimatedCost = Self::Potential;

    fn new(goal: &TWorld) -> Self
    where
        Self: Sized,
    {
        let goals = normalized_eq_worlds(goal)
            .map(centered_by_pos_median)
            .collect();
        Self { goals }
    }

    fn get_potential(&mut self, state: &TWorld) -> Self::Potential {
        let state = centered_by_pos_median::<TWorld, _>(state);

        assert!(!self.goals.is_empty());
        self.goals
            .iter()
            .map(|goal| compute_best_mapping_cost(&state, goal))
            .map(OrderedFloat)
            .min()
            .unwrap()
    }

    fn estimated_cost(cost: Cost<Self::Potential>) -> Self::EstimatedCost {
        cost.potential + OrderedFloat(cost.real_cost as f32).sqrt()
    }
}
