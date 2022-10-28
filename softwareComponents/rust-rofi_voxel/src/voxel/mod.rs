pub mod body;

pub use body::{VoxelBody, VoxelBodyWithPos};

use crate::pos::VoxelPos;
use modular_bitfield::prelude::*;

#[bitfield(bits = 7)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Voxel {
    value: VoxelBody,
    has_value: bool,
}
static_assertions::assert_eq_size!(Voxel, u8);

impl std::fmt::Debug for Voxel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        debug_assert!(
            self.has_body() || *self == Voxel::EMPTY,
            "Empty but non-zero voxel ({:?})",
            self.value()
        );
        self.get_body().fmt(f)
    }
}

pub type VoxelWithPos = (Voxel, VoxelPos);

impl Default for Voxel {
    fn default() -> Self {
        Self::new()
    }
}

impl Voxel {
    pub const EMPTY: Self = Self::new();

    pub fn new_with(body_opt: Option<VoxelBody>) -> Self {
        body_opt.map_or(Self::default(), Self::new_with_body)
    }
    pub fn new_with_body(body: VoxelBody) -> Self {
        Self::new().with_has_value(true).with_value(body)
    }
    pub fn has_body(self) -> bool {
        self.has_value()
    }
    pub fn get_body(self) -> Option<VoxelBody> {
        debug_assert!(
            self.has_body() || self == Voxel::EMPTY,
            "Empty but non-zero voxel ({:?})",
            self.value()
        );
        if self.has_body() {
            Option::Some(self.value())
        } else {
            Option::None
        }
    }

    pub fn clear_body(&mut self) {
        *self = Self::EMPTY;
    }
    pub fn set_body(&mut self, body: VoxelBody) {
        *self = Self::new_with_body(body);
    }
    pub fn replace_body(&mut self, body: VoxelBody) -> Self {
        std::mem::replace(self, Self::new_with_body(body))
    }
}

impl From<Option<VoxelBody>> for Voxel {
    fn from(body_opt: Option<VoxelBody>) -> Self {
        Self::new_with(body_opt)
    }
}
impl From<Voxel> for Option<VoxelBody> {
    fn from(voxel: Voxel) -> Self {
        voxel.get_body()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_empty_voxel() {
        assert_eq!(Voxel::default(), Voxel::EMPTY);
        let empty_voxel = Voxel::EMPTY;
        assert!(!empty_voxel.has_body());
        assert_eq!(empty_voxel.get_body(), Option::None);
    }
}
