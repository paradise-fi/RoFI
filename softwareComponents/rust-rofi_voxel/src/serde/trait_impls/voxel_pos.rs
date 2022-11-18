use crate::pos::RelativeVoxelPos;
use serde::{Deserialize, Serialize};

impl Serialize for RelativeVoxelPos {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let RelativeVoxelPos(pos) = self;
        pos.serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for RelativeVoxelPos {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer).map(Self)
    }
}
