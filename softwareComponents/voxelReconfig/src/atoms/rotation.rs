use super::{Axis, Direction};
use enum_iterator::Sequence;
use modular_bitfield::prelude::*;
use static_assertions::{assert_eq_size, const_assert_eq};

#[derive(
    BitfieldSpecifier, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Sequence, amplify::Display,
)]
#[display(Debug)]
#[repr(u8)]
#[bits = 1]
pub enum RotationAngle {
    Plus90,
    Minus90,
}
const_assert_eq!(RotationAngle::CARDINALITY, 2);

impl std::fmt::Debug for RotationAngle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            RotationAngle::Plus90 => write!(f, "+90"),
            RotationAngle::Minus90 => write!(f, "-90"),
        }
    }
}

impl RotationAngle {
    pub fn is_positive(self) -> bool {
        match self {
            RotationAngle::Plus90 => true,
            RotationAngle::Minus90 => false,
        }
    }

    pub fn opposite(self) -> Self {
        match self {
            RotationAngle::Plus90 => RotationAngle::Minus90,
            RotationAngle::Minus90 => RotationAngle::Plus90,
        }
    }
}

#[bitfield(bits = 3)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Rotation {
    pub axis: Axis,
    pub angle: RotationAngle,
}
assert_eq_size!(Rotation, u8);

impl Rotation {
    pub fn new_with(axis: Axis, angle: RotationAngle) -> Self {
        Self::new().with_axis(axis).with_angle(angle)
    }

    pub fn new_from_dir(direction: Direction, rot_angle: RotationAngle) -> Self {
        Self::new_with(
            direction.axis(),
            if direction.is_positive() {
                rot_angle
            } else {
                rot_angle.opposite()
            },
        )
    }

    pub fn new_from_to(from: Direction, to: Direction) -> Self {
        assert_ne!(from.axis(), to.axis());

        let result = if from.axis().next_axis() == to.axis() {
            let axis = from.axis().prev_axis();
            if from.is_positive() == to.is_positive() {
                Self::new_with(axis, RotationAngle::Plus90)
            } else {
                Self::new_with(axis, RotationAngle::Minus90)
            }
        } else {
            let axis = from.axis().next_axis();
            if from.is_positive() == to.is_positive() {
                Self::new_with(axis, RotationAngle::Minus90)
            } else {
                Self::new_with(axis, RotationAngle::Plus90)
            }
        };

        debug_assert_eq!(result.rotate_dir(from), to);

        result
    }

    pub fn rotate<T>(&self, position: [T; 3]) -> [T; 3]
    where
        T: std::ops::Neg<Output = T>,
    {
        let [x, y, z] = position;
        match (self.axis(), self.angle()) {
            (Axis::X, RotationAngle::Plus90) => [x, -z, y],
            (Axis::X, RotationAngle::Minus90) => [x, z, -y],
            (Axis::Y, RotationAngle::Plus90) => [z, y, -x],
            (Axis::Y, RotationAngle::Minus90) => [-z, y, x],
            (Axis::Z, RotationAngle::Plus90) => [-y, x, z],
            (Axis::Z, RotationAngle::Minus90) => [y, -x, z],
        }
    }

    pub fn rotate_sizes<T>(&self, sizes: [T; 3]) -> [T; 3] {
        let [x, y, z] = sizes;
        match self.axis() {
            Axis::X => [x, z, y],
            Axis::Y => [z, y, x],
            Axis::Z => [y, x, z],
        }
    }

    pub fn rotate_axis(&self, axis: Axis) -> Axis {
        if axis == self.axis() {
            axis
        } else if axis == self.axis().next_axis() {
            self.axis().prev_axis()
        } else if axis == self.axis().prev_axis() {
            self.axis().next_axis()
        } else {
            panic!()
        }
    }

    pub fn rotate_dir(&self, dir: Direction) -> Direction {
        let result_is_positive = if dir.axis() == self.axis() {
            dir.is_positive()
        } else {
            let is_next_axis = dir.axis() == self.axis().next_axis();
            is_next_axis ^ self.angle().is_positive() ^ dir.is_positive()
        };
        let result = Direction::new_with(self.rotate_axis(dir.axis()), result_is_positive);

        debug_assert_eq!(
            self.rotate(dir.update_position([0; 3])),
            result.update_position([0; 3])
        );

        result
    }

    pub fn inverse(&self) -> Self {
        Self::new_with(self.axis(), self.angle().opposite())
    }
}
