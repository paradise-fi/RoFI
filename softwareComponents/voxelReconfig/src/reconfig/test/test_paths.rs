use crate::atoms::{Axis, Direction};
use crate::voxel::{JointPosition, PosVoxel, Voxel};

pub fn no_steps_path<TIndex>() -> impl IntoIterator<Item = Vec<PosVoxel<TIndex>>>
where
    TIndex: num::Num + Copy,
{
    [vec![
        (
            [num::zero(); 3].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, true),
                true,
                JointPosition::Plus90,
            ),
        ),
        (
            [num::one(), num::zero(), num::zero()].into(),
            Voxel::new_with(
                Direction::new_with(Axis::X, false),
                false,
                JointPosition::Minus90,
            ),
        ),
    ]]
}

pub fn one_step_path<TIndex>() -> impl IntoIterator<Item = Vec<PosVoxel<TIndex>>>
where
    TIndex: num::Num + Copy,
{
    [
        vec![
            (
                [num::zero(); 3].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [num::one(), num::zero(), num::zero()].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, false),
                    false,
                    JointPosition::Minus90,
                ),
            ),
        ],
        vec![
            (
                [num::zero(); 3].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [num::one(), num::zero(), num::zero()].into(),
                Voxel::new_with(
                    Direction::new_with(Axis::X, false),
                    true,
                    JointPosition::Minus90,
                ),
            ),
        ],
    ]
}
