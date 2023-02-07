use crate::voxel;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize)]
#[serde(try_from = "i32")]
#[serde(into = "i32")]
#[serde(deny_unknown_fields)]
enum ShoeJointPosition {
    #[default]
    Zero,
    Plus90,
    Minus90,
}

impl From<ShoeJointPosition> for i32 {
    fn from(pos: ShoeJointPosition) -> Self {
        match pos {
            ShoeJointPosition::Zero => 0,
            ShoeJointPosition::Plus90 => 90,
            ShoeJointPosition::Minus90 => -90,
        }
    }
}
impl TryFrom<i32> for ShoeJointPosition {
    type Error = String;
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Zero),
            90 => Ok(Self::Plus90),
            -90 => Ok(Self::Minus90),
            _ => Err(format!("Shoe joint pos {value} is not valid")),
        }
    }
}

impl From<voxel::body::JointPosition> for ShoeJointPosition {
    fn from(value: voxel::body::JointPosition) -> Self {
        match value {
            voxel::body::JointPosition::Zero => Self::Zero,
            voxel::body::JointPosition::Plus90 => Self::Plus90,
            voxel::body::JointPosition::Minus90 => Self::Minus90,
        }
    }
}
impl From<ShoeJointPosition> for voxel::body::JointPosition {
    fn from(value: ShoeJointPosition) -> Self {
        match value {
            ShoeJointPosition::Zero => Self::Zero,
            ShoeJointPosition::Plus90 => Self::Plus90,
            ShoeJointPosition::Minus90 => Self::Minus90,
        }
    }
}

impl Serialize for voxel::body::JointPosition {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        ShoeJointPosition::from(*self).serialize(serializer)
    }
}
impl<'de> Deserialize<'de> for voxel::body::JointPosition {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        ShoeJointPosition::deserialize(deserializer).map(Self::from)
    }
}
