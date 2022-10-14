use crate::pos::VoxelPos;
use serde::{Deserialize, Serialize};

impl Serialize for VoxelPos {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let VoxelPos(pos) = self;
        pos.serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for VoxelPos {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer).map(Self)
    }
}
