use super::VoxelWorld;
use crate::pos::{Pos, SizeRanges};
use crate::voxel::Voxel;

pub struct CenteredVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    world: TWorldRef,
    center: Pos<TWorld::IndexType>,
}

impl<TWorld, TWorldRef> std::fmt::Debug for CenteredVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
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

        f.write_fmt(format_args!("{indent}voxels: "))?;
        super::debug_fmt_voxels(self.all_voxels(), f, ws_sep, indent, indent, f.alternate())?;

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<TWorld, TWorldRef> CenteredVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    pub fn new(world: TWorldRef, center: Pos<TWorld::IndexType>) -> Self {
        Self { world, center }
    }

    fn world(&self) -> &TWorld {
        self.world.borrow()
    }

    fn center(&self) -> Pos<TWorld::IndexType> {
        self.center
    }

    fn get_new_pos(&self, orig_pos: Pos<TWorld::IndexType>) -> Pos<TWorld::IndexType> {
        orig_pos - self.center()
    }
    fn get_orig_pos(&self, new_pos: Pos<TWorld::IndexType>) -> Pos<TWorld::IndexType> {
        new_pos + self.center()
    }
}

impl<TWorld, TWorldRef> VoxelWorld for CenteredVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    type IndexType = TWorld::IndexType;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
        let (start, end) = self.world().size_ranges().start_end();
        SizeRanges::new(self.get_new_pos(start), self.get_new_pos(end))
    }

    fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
        Box::new(
            self.world()
                .all_voxels()
                .map(|(orig_pos, voxel)| (self.get_new_pos(orig_pos), voxel)),
        )
    }

    fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel> {
        let orig_pos = self.get_orig_pos(pos);
        self.world().get_voxel(orig_pos)
    }
}
