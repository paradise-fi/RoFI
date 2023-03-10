pub use complement::ComplementVoxelSubworld;

use super::VoxelWorld;
use crate::pos::ord::OrdPos;
use crate::pos::{minimal_pos_hull, Pos, SizeRanges};
use crate::voxel::{PosVoxel, Voxel};
use iter_fixed::IntoIteratorFixed;
use std::collections::BTreeSet;

/// Can represent a world that is not valid (has voxels with missing other body)
pub struct VoxelSubworld<'a, TWorld: VoxelWorld> {
    world: &'a TWorld,
    included: BTreeSet<OrdPos<TWorld::IndexType>>,
    size_ranges: SizeRanges<TWorld::IndexType>,
    complement_size_ranges: SizeRanges<TWorld::IndexType>,
}

impl<'a, TWorld: VoxelWorld> std::fmt::Debug for VoxelSubworld<'a, TWorld> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("VoxelSubworld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!(
            "{indent}sizes: {:?},{ws_sep}",
            self.size_ranges()
        ))?;

        f.write_fmt(format_args!("{indent}voxels: "))?;
        super::debug_fmt_voxels(self.all_voxels(), f, ws_sep, indent, indent, f.alternate())?;

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<'a, TWorld: VoxelWorld> VoxelSubworld<'a, TWorld> {
    pub fn new<P: Fn(Pos<TWorld::IndexType>) -> bool>(world: &'a TWorld, predicate: P) -> Self {
        let included = world
            .all_voxels()
            .filter(|&(pos, _)| predicate(pos))
            .map(|(pos, _)| OrdPos(pos))
            .collect::<BTreeSet<_>>();

        Self::new_from_set_unchecked(world, included)
    }

    pub fn new_from_set(
        world: &'a TWorld,
        mut included: BTreeSet<OrdPos<TWorld::IndexType>>,
    ) -> Self {
        included.retain(|&OrdPos(pos)| world.get_voxel(pos).is_some());
        Self::new_from_set_unchecked(world, included)
    }

    fn new_from_set_unchecked(
        world: &'a TWorld,
        included: BTreeSet<OrdPos<TWorld::IndexType>>,
    ) -> Self {
        debug_assert!(included
            .iter()
            .all(|&OrdPos(pos)| world.get_voxel(pos).is_some()));

        let size_ranges = minimal_pos_hull(included.iter().map(|&OrdPos(pos)| pos));
        let complement_size_ranges = minimal_pos_hull(
            world
                .all_voxels()
                .map(|(pos, _)| pos)
                .filter(|&pos| !included.contains(&OrdPos(pos))),
        );

        debug_assert_eq!(
            size_ranges
                .as_ranges_array()
                .into_iter_fixed()
                .zip(complement_size_ranges.as_ranges_array())
                .map(|(lhs, rhs)| {
                    std::cmp::min(lhs.start, rhs.start)..std::cmp::max(lhs.end, rhs.end)
                })
                .collect::<[_; 3]>(),
            world.size_ranges().as_ranges_array(),
        );

        Self {
            world,
            included,
            size_ranges,
            complement_size_ranges,
        }
    }

    fn contains(&self, pos: Pos<<Self as VoxelWorld>::IndexType>) -> bool {
        self.included.contains(&OrdPos(pos))
    }

    pub fn underlying_world(&self) -> &'a TWorld {
        self.world
    }

    pub fn complement(&self) -> ComplementVoxelSubworld<'a, TWorld, &Self> {
        ComplementVoxelSubworld::new(self)
    }
}

impl<'a, TWorld: VoxelWorld> VoxelWorld for VoxelSubworld<'a, TWorld> {
    type IndexType = TWorld::IndexType;
    type PosVoxelIter<'b> = impl Iterator<Item = PosVoxel<Self::IndexType>> where Self: 'b;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
        self.size_ranges
    }

    fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
        Box::new(
            self.included
                .iter()
                .map(|&OrdPos(pos)| (pos, self.world.get_voxel(pos).unwrap())),
        )
    }

    fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel> {
        if self.included.contains(&OrdPos(pos)) {
            self.world.get_voxel(pos)
        } else {
            None
        }
    }
}

pub mod complement {
    use super::VoxelSubworld;
    use crate::pos::{Pos, SizeRanges};
    use crate::voxel::{PosVoxel, Voxel};
    use crate::voxel_world::{debug_fmt_voxels, VoxelWorld};
    use std::marker::PhantomData;

    /// Can represent a world that is not valid (has voxels with missing other body)
    pub struct ComplementVoxelSubworld<'a, TWorld, TWorldRef>(TWorldRef, PhantomData<&'a TWorld>)
    where
        TWorld: 'a + VoxelWorld,
        TWorldRef: std::borrow::Borrow<VoxelSubworld<'a, TWorld>>;

    impl<'a, TWorld, TWorldRef> std::fmt::Debug for ComplementVoxelSubworld<'a, TWorld, TWorldRef>
    where
        TWorld: VoxelWorld,
        TWorldRef: std::borrow::Borrow<VoxelSubworld<'a, TWorld>>,
    {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            let indent = if f.alternate() { "    " } else { "" };
            let ws_sep = if f.alternate() { "\n" } else { " " };
            f.write_str("ComplementVoxelSubworld {")?;
            f.write_str(ws_sep)?;

            f.write_fmt(format_args!(
                "{indent}sizes: {:?},{ws_sep}",
                self.size_ranges()
            ))?;

            f.write_fmt(format_args!("{indent}voxels: "))?;
            debug_fmt_voxels(self.all_voxels(), f, ws_sep, indent, indent, f.alternate())?;

            if f.alternate() {
                f.write_str(",")?;
            }
            f.write_str(ws_sep)?;
            f.write_str("}")
        }
    }

    impl<'a, TWorld, TWorldRef> ComplementVoxelSubworld<'a, TWorld, TWorldRef>
    where
        TWorld: 'a + VoxelWorld,
        TWorldRef: std::borrow::Borrow<VoxelSubworld<'a, TWorld>>,
    {
        pub fn new(subworld: TWorldRef) -> Self {
            Self(subworld, PhantomData::default())
        }

        fn complement(&self) -> &VoxelSubworld<'a, TWorld> {
            self.0.borrow()
        }

        fn underlying_world(&self) -> &'a TWorld {
            self.0.borrow().underlying_world()
        }

        fn contains(&self, pos: Pos<<Self as VoxelWorld>::IndexType>) -> bool {
            !self.complement().contains(pos)
        }
    }

    impl<'a, TWorld, TWorldRef> VoxelWorld for ComplementVoxelSubworld<'a, TWorld, TWorldRef>
    where
        TWorld: 'a + VoxelWorld,
        TWorldRef: std::borrow::Borrow<VoxelSubworld<'a, TWorld>>,
    {
        type IndexType = TWorld::IndexType;
        type PosVoxelIter<'b> = impl Iterator<Item = PosVoxel<Self::IndexType>> where Self: 'b;

        #[allow(clippy::misnamed_getters)] // Getting size_range of the complement's complement
        fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
            self.complement().complement_size_ranges
        }

        fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
            Box::new(
                self.underlying_world()
                    .all_voxels()
                    .filter(|&(pos, _)| self.contains(pos)),
            )
        }

        fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel> {
            if self.contains(pos) {
                Some(
                    self.underlying_world()
                        .get_voxel(pos)
                        .expect("Invalid subworld"),
                )
            } else {
                None
            }
        }
    }
}
