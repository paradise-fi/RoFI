use super::{IndexType, VoxelPos};

pub type RelativeIndexType = i8;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct RelativeVoxelPos(pub [RelativeIndexType; 3]);

#[derive(Debug, amplify::Display, amplify::Error)]
#[display(doc_comments)]
pub enum Error {
    /// Trying to create VoxelPos with negative values ({0:?})
    VoxelPosWithNegValues(RelativeVoxelPos),
}

impl RelativeVoxelPos {
    fn cast_from_abs(abs_pos: VoxelPos) -> Self {
        let VoxelPos(abs_pos) = abs_pos;
        let rel_pos = abs_pos.map(RelativeIndexType::try_from).map(Result::unwrap);
        Self(rel_pos)
    }
    fn cast_to_abs(self) -> Result<VoxelPos, Error> {
        let Self(rel_pos) = self;
        if rel_pos.iter().any(|&pos| pos < 0) {
            return Err(Error::VoxelPosWithNegValues(self));
        }
        let abs_pos = rel_pos.map(IndexType::try_from).map(Result::unwrap);
        Ok(VoxelPos(abs_pos))
    }

    pub fn from_pos_and_origin(pos: VoxelPos, origin: VoxelPos) -> Self {
        let Self(pos) = Self::cast_from_abs(pos);
        let Self(origin) = Self::cast_from_abs(origin);

        let new_pos = pos.zip(origin).map(|(pos, origin)| pos - origin);
        Self(new_pos)
    }
    pub fn to_abs_pos(self, origin: VoxelPos) -> Result<VoxelPos, Error> {
        let RelativeVoxelPos(rel_pos) = self;
        let RelativeVoxelPos(origin) = Self::cast_from_abs(origin);

        let new_pos = rel_pos.zip(origin).map(|(pos, origin)| origin + pos);
        Self(new_pos).cast_to_abs()
    }
}
