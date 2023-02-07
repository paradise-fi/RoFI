use super::VoxelSubworld;
use crate::pos::{RelativeIndexType, RelativeVoxelPos, VoxelPos};
use crate::voxel::{Voxel, VoxelBody};

pub struct CenteredVoxelWorld<'a> {
    center: VoxelPos,
    world: VoxelSubworld<'a>,
}

impl<'a> std::fmt::Debug for CenteredVoxelWorld<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("CenteredVoxelWorld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!("{indent}center: {:?},{ws_sep}", self.center()))?;

        f.write_fmt(format_args!(
            "{indent}sizes: {:?},{ws_sep}",
            self.size_ranges()
        ))?;

        f.write_fmt(format_args!("{indent}bodies: "))?;
        super::debug_fmt_bodies(self.all_bodies(), f, ws_sep, indent, indent, f.alternate())?;

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<'a> CenteredVoxelWorld<'a> {
    pub fn new(world: VoxelSubworld<'a>, center: VoxelPos) -> Self {
        let VoxelPos(c) = center;
        assert!(
            c.iter().all(|&i| RelativeIndexType::try_from(i).is_ok()),
            "Abs pos is too large"
        );
        Self { center, world }
    }

    pub fn center(&self) -> VoxelPos {
        self.center
    }

    pub fn size_ranges(&self) -> [std::ops::Range<RelativeIndexType>; 3] {
        let world_size_ranges = self.world.size_ranges();
        let min_pos = world_size_ranges.each_ref().map(|size_i| size_i.start);
        let max_pos = world_size_ranges.each_ref().map(|size_i| size_i.end);
        let RelativeVoxelPos(min_pos) = self.get_rel_pos(VoxelPos(min_pos));
        let RelativeVoxelPos(max_pos) = self.get_rel_pos(VoxelPos(max_pos));
        min_pos.zip(max_pos).map(|(min, max)| min..max)
    }

    fn get_rel_pos(&self, abs_pos: VoxelPos) -> RelativeVoxelPos {
        RelativeVoxelPos::from_pos_and_origin(abs_pos, self.center)
    }

    fn get_abs_pos(&self, rel_pos: RelativeVoxelPos) -> Result<VoxelPos, impl std::error::Error> {
        rel_pos.to_abs_pos(self.center)
    }

    pub fn get_voxel(&self, pos: RelativeVoxelPos) -> Option<Voxel> {
        self.world.get_voxel(self.get_abs_pos(pos).ok()?)
    }

    pub fn get_body(&self, pos: RelativeVoxelPos) -> Option<VoxelBody> {
        self.get_voxel(pos)?.get_body()
    }

    pub fn all_bodies(&self) -> impl Iterator<Item = (VoxelBody, RelativeVoxelPos)> + '_ {
        self.world.all_bodies().map(|(body, abs_pos)| {
            let rel_pos = self.get_rel_pos(abs_pos);
            (body, rel_pos)
        })
    }
}
