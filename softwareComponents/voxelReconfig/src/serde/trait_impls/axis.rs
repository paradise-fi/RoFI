use crate::atoms;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
enum Axis {
    X,
    Y,
    Z,
}

impl From<Axis> for atoms::Axis {
    fn from(value: Axis) -> Self {
        match value {
            Axis::X => Self::X,
            Axis::Y => Self::Y,
            Axis::Z => Self::Z,
        }
    }
}
impl From<atoms::Axis> for Axis {
    fn from(value: atoms::Axis) -> Self {
        match value {
            atoms::Axis::X => Self::X,
            atoms::Axis::Y => Self::Y,
            atoms::Axis::Z => Self::Z,
        }
    }
}

impl Serialize for atoms::Axis {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        Axis::from(*self).serialize(serializer)
    }
}
impl<'de> Deserialize<'de> for atoms::Axis {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Axis::deserialize(deserializer).map(Self::from)
    }
}
