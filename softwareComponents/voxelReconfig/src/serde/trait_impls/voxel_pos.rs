use crate::pos::Pos;
use serde::{Deserialize, Serialize};

impl<TIndex: num::Num + Serialize> Serialize for Pos<TIndex> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        self.as_array_ref().serialize(serializer)
    }
}

impl<'de, TIndex: num::Num + Deserialize<'de>> Deserialize<'de> for Pos<TIndex> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer).map(<[TIndex; 3]>::into)
    }
}
