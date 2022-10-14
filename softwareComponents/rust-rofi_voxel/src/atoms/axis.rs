use enum_iterator::Sequence;
use modular_bitfield::prelude::*;

#[derive(BitfieldSpecifier, Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Sequence)]
#[repr(u8)]
#[bits = 2]
pub enum Axis {
    X = 0,
    Y,
    Z,
}

impl Axis {
    pub fn as_index(self) -> u8 {
        match self {
            Axis::X => 0,
            Axis::Y => 1,
            Axis::Z => 2,
        }
    }

    pub fn next_axis(self) -> Self {
        enum_iterator::next_cycle(&self).unwrap()
    }

    pub fn prev_axis(self) -> Self {
        enum_iterator::previous_cycle(&self).unwrap()
    }
}

mod test {
    #[test]
    fn test_axis_as_index() {
        for (axis, i) in enum_iterator::all::<super::Axis>().zip(0..) {
            assert_eq!(axis.as_index(), i);
        }
    }
}
