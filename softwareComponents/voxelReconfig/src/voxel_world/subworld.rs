use super::VoxelWorld;
use crate::pos::{compute_minimal_pos_hull, VoxelPos};
use crate::voxel::{Voxel, VoxelBody};
use std::collections::HashSet;

/// Can represent a world that is not valid (has bodies with missing other body)
#[derive(Clone)]
pub struct VoxelSubworld<'a> {
    world: &'a VoxelWorld,
    included: HashSet<VoxelPos>,
    size_ranges: [std::ops::Range<u8>; 3],
}

impl<'a> std::fmt::Debug for VoxelSubworld<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("VoxelSubworld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!(
            "{indent}sizes: {:?},{ws_sep}",
            self.size_ranges()
        ))?;

        f.write_fmt(format_args!("{indent}bodies: "))?;
        super::debug_fmt_bodies(self.all_bodies(), f, ws_sep, indent, indent, f.alternate())?;
        f.write_fmt(format_args!(",{ws_sep}"))?;

        {
            let complement = self.complement();
            let complement_indent = if f.alternate() { "        " } else { "" };
            f.write_fmt(format_args!("{indent}complement: VoxelSubworld {{"))?;
            f.write_str(ws_sep)?;

            f.write_fmt(format_args!(
                "{complement_indent}sizes: {:?},{ws_sep}",
                complement.size_ranges()
            ))?;

            f.write_fmt(format_args!("{complement_indent}bodies: "))?;
            super::debug_fmt_bodies(
                complement.all_bodies(),
                f,
                ws_sep,
                complement_indent,
                indent,
                f.alternate(),
            )?;

            if f.alternate() {
                f.write_str(",")?;
            }
            f.write_str(ws_sep)?;
            f.write_str(indent)?;
            f.write_str("}")?;
        }

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<'a> VoxelSubworld<'a> {
    pub fn new<P: Fn(VoxelPos) -> bool>(world: &'a VoxelWorld, predicate: P) -> Self {
        let included = world
            .all_bodies()
            .map(|(_, pos)| pos)
            .filter(|&pos| predicate(pos))
            .collect::<HashSet<_>>();

        let size_ranges = compute_minimal_pos_hull(included.iter().map(|&VoxelPos(pos)| pos))
            .expect("Subworld cannot be empty");

        Self {
            world,
            included,
            size_ranges,
        }
    }

    pub fn underlying_world(&self) -> &'a VoxelWorld {
        self.world
    }

    pub fn complement(&self) -> Self {
        Self::new(self.world, |pos| !self.included.contains(&pos))
    }

    pub fn size_ranges(&self) -> [std::ops::Range<u8>; 3] {
        self.size_ranges.clone()
    }

    pub fn all_bodies(&self) -> impl Iterator<Item = (VoxelBody, VoxelPos)> + '_ {
        self.world
            .all_bodies()
            .filter(|&(_, pos)| self.included.contains(&pos))
    }

    pub fn get_voxel(&self, position: VoxelPos) -> Option<Voxel> {
        self.world.get_voxel(position).map(|voxel| {
            if self.included.contains(&position) {
                voxel
            } else {
                Voxel::EMPTY
            }
        })
    }

    pub fn get_body(&self, position: VoxelPos) -> Option<VoxelBody> {
        self.get_voxel(position)?.get_body()
    }
}
