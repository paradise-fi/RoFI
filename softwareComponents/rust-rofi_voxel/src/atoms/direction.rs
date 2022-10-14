use super::Axis;
use modular_bitfield::prelude::*;

#[bitfield(bits = 3)]
#[derive(BitfieldSpecifier, Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Direction {
    pub axis: Axis,
    pub is_positive: bool,
}

impl Direction {
    pub fn new_with(axis: Axis, is_positive: bool) -> Self {
        Self::new().with_axis(axis).with_is_positive(is_positive)
    }

    pub fn update_position<T>(self, mut position: [T; 3]) -> Result<[T; 3], String>
    where
        T: num::One + num::CheckedAdd + num::CheckedSub,
    {
        let axis_pos = &mut position[usize::from(self.axis().as_index())];
        if self.is_positive() {
            *axis_pos = axis_pos
                .checked_add(&T::one())
                .ok_or("Direction is towards upper num bound")?;
        } else {
            *axis_pos = axis_pos
                .checked_sub(&T::one())
                .ok_or("Direction is towards lower num bound")?;
        }
        Ok(position)
    }

    pub fn opposite(self) -> Self {
        self.with_is_positive(!self.is_positive())
    }

    pub fn from_adjacent_positions<T>(from: [T; 3], to: [T; 3]) -> Option<Self>
    where
        T: num::One + num::CheckedAdd + num::CheckedSub + PartialOrd,
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

        if result.update_position(from) == Ok(to) {
            Some(result)
        } else {
            None
        }
    }
}
