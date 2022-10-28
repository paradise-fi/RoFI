use super::VoxelWorld;
use crate::pos::{compute_minimal_pos_hull, VoxelPos};
use crate::voxel::{Voxel, VoxelBody};
use std::collections::HashSet;

/// Can represent a world that is not valid (has bodies with missing other body)
#[derive(Debug, Clone)]
pub struct VoxelSubworld<'a> {
    world: &'a VoxelWorld,
    included: HashSet<VoxelPos>,
    size_ranges: [std::ops::Range<u8>; 3],
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

    pub fn dump_bodies(&self) {
        println!("VoxelSubworld {{ bodies=[");
        for (body, pos) in self.all_bodies() {
            println!("    {:?}: {:?},", pos, body);
        }
        println!("] }}");
    }
}
