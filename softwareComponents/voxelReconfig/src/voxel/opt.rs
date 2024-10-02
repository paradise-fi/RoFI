use super::Voxel;
use modular_bitfield::prelude::*;
use static_assertions::assert_eq_size;

#[bitfield(bits = 7)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct VoxelOpt {
    value: Voxel,
    has_value: bool,
}
assert_eq_size!(VoxelOpt, u8);

impl std::fmt::Debug for VoxelOpt {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        debug_assert!(
            self.has_voxel() || *self == VoxelOpt::EMPTY,
            "Empty but non-zero value ({:?})",
            self.value()
        );
        self.get_voxel().fmt(f)
    }
}

impl Default for VoxelOpt {
    fn default() -> Self {
        Self::new()
    }
}

impl VoxelOpt {
    pub const EMPTY: Self = Self::new();

    pub fn new_with(voxel: Voxel) -> Self {
        Self::new().with_has_value(true).with_value(voxel)
    }
    pub fn has_voxel(self) -> bool {
        self.has_value()
    }
    pub fn get_voxel(self) -> Option<Voxel> {
        debug_assert!(
            self.has_voxel() || self == VoxelOpt::EMPTY,
            "Empty but non-zero value ({:?})",
            self.value()
        );
        if self.has_voxel() {
            Option::Some(self.value())
        } else {
            Option::None
        }
    }

    pub fn clear_voxel(&mut self) {
        *self = Self::EMPTY;
    }
    pub fn set_voxel(&mut self, voxel: Voxel) {
        *self = Self::new_with(voxel);
    }
    pub fn replace_voxel(&mut self, voxel: Voxel) -> Self {
        std::mem::replace(self, Self::new_with(voxel))
    }
}

impl From<Option<Voxel>> for VoxelOpt {
    fn from(voxel_opt: Option<Voxel>) -> Self {
        voxel_opt.map_or(VoxelOpt::default(), Self::new_with)
    }
}
impl From<VoxelOpt> for Option<Voxel> {
    fn from(voxel_opt: VoxelOpt) -> Self {
        voxel_opt.get_voxel()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_empty_voxel() {
        assert_eq!(VoxelOpt::default(), VoxelOpt::EMPTY);
        let empty_voxel = VoxelOpt::EMPTY;
        assert!(!empty_voxel.has_voxel());
        assert_eq!(empty_voxel.get_voxel(), Option::None);
    }
}
