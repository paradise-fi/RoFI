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
        let [from_x, from_y, from_z] = &from;
        let [to_x, to_y, to_z] = &to;

        let result = if from_x != to_x {
            Self::new_with(Axis::X, to_x > from_x)
        } else if from_y != to_y {
            Self::new_with(Axis::Y, to_y > from_y)
        } else if from_z != to_z {
            Self::new_with(Axis::Z, to_z > from_z)
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

#[test]
fn test_from_adjacent_positions() {
    for from in [[0; 3], [3; 3], [-5; 3], [1, 2, 3], [5, -3, -1]] {
        for change_axis in [Axis::X, Axis::Y, Axis::Z] {
            for is_positive in [true, false] {
                let mut to = from;
                to[change_axis.as_index()] += if is_positive { 1 } else { -1 };

                let result = Direction::from_adjacent_positions(from, to);
                assert_eq!(
                    result,
                    Some(Direction::new_with(change_axis, is_positive)),
                    "from: {from:?}, to: {to:?}"
                );
            }
        }
    }
}
