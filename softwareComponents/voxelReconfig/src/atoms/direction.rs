use super::Axis;
use modular_bitfield::prelude::*;

#[bitfield(bits = 3)]
#[derive(
    BitfieldSpecifier, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, amplify::Display,
)]
#[display(Debug)]
pub struct Direction {
    pub axis: Axis,
    pub is_positive: bool,
}
impl std::fmt::Debug for Direction {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let sign = if self.is_positive() { '+' } else { '-' };
        write!(f, "{}{}", self.axis(), sign)
    }
}

impl Direction {
    pub fn new_with(axis: Axis, is_positive: bool) -> Self {
        Self::new().with_axis(axis).with_is_positive(is_positive)
    }

    pub fn update_position<T>(self, mut position: [T; 3]) -> [T; 3]
    where
        T: num::Signed,
    {
        let axis_pos = &mut position[self.axis().as_index()];
        if self.is_positive() {
            *axis_pos = std::mem::replace(axis_pos, num::zero()) + num::one();
        } else {
            *axis_pos = std::mem::replace(axis_pos, num::zero()) - num::one();
        }
        position
    }

    pub fn opposite(self) -> Self {
        self.with_is_positive(!self.is_positive())
    }

    pub fn from_adjacent_positions<T>(from: [T; 3], to: [T; 3]) -> Option<Self>
    where
        T: num::Signed + Ord + Copy,
    {
        let [first_x, first_y, first_z] = &from;
        let [second_x, second_y, second_z] = &to;

        let result = if first_x != second_x {
            Self::new_with(Axis::X, first_x > second_x)
        } else if first_y != second_y {
            Self::new_with(Axis::Y, first_y > second_y)
        } else if first_z != second_z {
            Self::new_with(Axis::Z, first_z > second_z)
        } else {
            return None;
        };

        if result.update_position(from) == to {
            Some(result)
        } else {
            None
        }
    }
}
