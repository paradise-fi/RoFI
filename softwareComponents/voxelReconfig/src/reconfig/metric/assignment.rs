//! Metric based on best assignment algorithm.
//! This limits the metric to evaluating only a pair of modules.

use super::centered_by_pos_median;
use super::potential_fn::{FSqrtSum, PotentialFn};
use crate::module_repr::{get_all_module_reprs, get_other_body, is_module_repr};
use crate::pos::Pos;
use crate::voxel::{JointPosition, PosVoxel, Voxel};
use crate::voxel_world::normalized_eq_worlds;
use crate::voxel_world::{CenteredVoxelWorld, NormVoxelWorld, VoxelWorld};
use iter_fixed::IntoIteratorFixed;
use ndarray::Array2;
use num::ToPrimitive;
use ordered_float::OrderedFloat;
use std::marker::PhantomData;

fn voxel_joint_diff<N: num::Zero + num::One>(goal: Voxel, other: Voxel) -> N {
    match (goal.joint_pos(), other.joint_pos()) {
        (JointPosition::Zero, JointPosition::Zero) => num::zero(),
        (JointPosition::Zero, _) | (_, JointPosition::Zero) => num::one(),
        _ => num::zero(),
    }
}
pub fn joint_diff<N: num::Num + PartialOrd>(goal: [Voxel; 2], other: [Voxel; 2]) -> N {
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

fn compute_best_mapping_cost<TWorld, TGoalWorld, TCostFn>(other: &TWorld, goal: &TGoalWorld) -> f32
where
    TWorld: VoxelWorld,
    TGoalWorld: VoxelWorld<IndexType = TWorld::IndexType>,
    TWorld::IndexType: ToPrimitive,
    TCostFn: CostFn<TWorld::IndexType>,
{
    let other_modules_count = get_all_module_reprs(other).count();
    let goal_modules_count = get_all_module_reprs(goal).count();
    assert_eq!(other_modules_count, goal_modules_count);

    let mut costs = Array2::zeros((other_modules_count, goal_modules_count));
    for (other_body_a, i) in get_all_module_reprs(other).zip(0..) {
        let other_body_b = get_other_body(other_body_a, other).unwrap();
        for (goal_body_a, j) in get_all_module_reprs(goal).zip(0..) {
            let goal_body_b = get_other_body(goal_body_a, goal).unwrap();

            costs[(i, j)] =
                TCostFn::compute_cost([other_body_a, other_body_b], [goal_body_a, goal_body_b]);
        }
    }

    let best_mapping = lapjv::lapjv(&costs).unwrap();
    lapjv::cost(&costs, &best_mapping.0)
}

pub trait CostFn<TIndex: num::Num> {
    fn compute_cost(lhs_module: [PosVoxel<TIndex>; 2], rhs_module: [PosVoxel<TIndex>; 2]) -> f32;
}

pub struct PosCostFn;
impl<TIndex> CostFn<TIndex> for PosCostFn
where
    TIndex: num::Signed + ToPrimitive + Copy,
{
    fn compute_cost(lhs_module: [PosVoxel<TIndex>; 2], rhs_module: [PosVoxel<TIndex>; 2]) -> f32 {
        debug_assert!(is_module_repr(lhs_module[0].1));
        debug_assert!(is_module_repr(rhs_module[0].1));
        // Comparing shoeA with shoeB will always have equal or higher pos cost
        let pos_cost =
            pos_cost(lhs_module[0].0, rhs_module[0].0) + pos_cost(lhs_module[1].0, rhs_module[1].0);
        ToPrimitive::to_f32(&pos_cost).expect("Invalid position cost")
    }
}

pub struct JointCostFn;
impl<TIndex> CostFn<TIndex> for JointCostFn
where
    TIndex: num::Signed,
{
    fn compute_cost(lhs_module: [PosVoxel<TIndex>; 2], rhs_module: [PosVoxel<TIndex>; 2]) -> f32 {
        f32::min(
            joint_diff(
                [lhs_module[0].1, lhs_module[1].1],
                [rhs_module[0].1, rhs_module[1].1],
            ),
            joint_diff(
                [lhs_module[1].1, lhs_module[0].1],
                [rhs_module[0].1, rhs_module[1].1],
            ),
        )
    }
}

pub struct PosJointCostFn;
impl<TIndex> CostFn<TIndex> for PosJointCostFn
where
    TIndex: num::Signed + ToPrimitive + Copy,
{
    fn compute_cost(lhs_module: [PosVoxel<TIndex>; 2], rhs_module: [PosVoxel<TIndex>; 2]) -> f32 {
        let pos_cost = PosCostFn::compute_cost(lhs_module, rhs_module);
        let joint_cost = JointCostFn::compute_cost(lhs_module, rhs_module);
        (pos_cost + joint_cost) / 2.
    }
}

pub struct AssignmentPotentialFn<TWorld, TCostFn>
where
    TWorld: VoxelWorld,
    TCostFn: CostFn<TWorld::IndexType>,
{
    goals: Vec<CenteredVoxelWorld<TWorld, TWorld>>,
    __phantom: PhantomData<TCostFn>,
}

impl<TWorld, TCostFn> PotentialFn<TWorld> for AssignmentPotentialFn<TWorld, TCostFn>
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: ToPrimitive,
    TCostFn: CostFn<TWorld::IndexType>,
{
    type Potential = OrderedFloat<f32>;

    fn new(goal: &TWorld) -> Self {
        let goals = normalized_eq_worlds(goal)
            .map(centered_by_pos_median)
            .collect();
        Self {
            goals,
            __phantom: Default::default(),
        }
    }

    fn get_potential(&mut self, state: &TWorld) -> Self::Potential {
        let state = centered_by_pos_median::<TWorld, _>(state);

        assert!(!self.goals.is_empty());
        self.goals
            .iter()
            .map(|goal| compute_best_mapping_cost::<_, _, TCostFn>(&state, goal))
            .map(OrderedFloat)
            .min()
            .unwrap()
    }
}

pub type PosAssgPotFn<TWorld> = AssignmentPotentialFn<TWorld, PosCostFn>;
pub type JointAssgPotFn<TWorld> = AssignmentPotentialFn<TWorld, JointCostFn>;
pub type PosJointAssgPotFn<TWorld> = AssignmentPotentialFn<TWorld, PosJointCostFn>;

pub type PosAssgMetric<TWorld> = FSqrtSum<TWorld, PosAssgPotFn<TWorld>>;
pub type JointAssgMetric<TWorld> = FSqrtSum<TWorld, JointAssgPotFn<TWorld>>;
pub type PosJointAssgMetric<TWorld> = FSqrtSum<TWorld, PosJointAssgPotFn<TWorld>>;
