use crate::atoms;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
struct Direction {
    pub axis: atoms::Axis,
    pub is_positive: bool,
}

impl From<Direction> for atoms::Direction {
    fn from(value: Direction) -> Self {
        let Direction { axis, is_positive } = value;
        Self::new_with(axis, is_positive)
    }
}
impl From<atoms::Direction> for Direction {
    fn from(value: atoms::Direction) -> Self {
        Self {
            axis: value.axis(),
            is_positive: value.is_positive(),
        }
    }
}

impl Serialize for atoms::Direction {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        Direction::from(*self).serialize(serializer)
    }
}
impl<'de> Deserialize<'de> for atoms::Direction {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Direction::deserialize(deserializer).map(Self::from)
    }
}
