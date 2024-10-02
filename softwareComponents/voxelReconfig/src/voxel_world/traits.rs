use super::*;
use crate::pos::{Pos, SizeRanges, Sizes};
use crate::voxel::{PosVoxel, Voxel};

pub trait VoxelWorld {
    type IndexType: std::fmt::Debug + num::Signed + Ord + Copy;
    type PosVoxelIter<'a>: Iterator<Item = PosVoxel<Self::IndexType>>
    where
        Self: 'a;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType>;
    fn all_voxels(&self) -> Self::PosVoxelIter<'_>;

    /// Returns `None` if `pos` position doesn't contain a voxel or is out of bounds
    fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel>;
}

pub trait NormVoxelWorld: VoxelWorld {
    /// Can be a world without the other bodies.
    ///
    /// `0..sizes` has to be a strict bound on the positions in voxels
    fn partial_from_sizes_and_voxels<IVoxels>(
        sizes: Sizes<Self::IndexType>,
        voxels: IVoxels,
    ) -> Result<Self, InvalidVoxelWorldError<Self::IndexType>>
    where
        Self: Sized,
        IVoxels: IntoIterator<Item = PosVoxel<Self::IndexType>>;
    /// `0..sizes` has to be a strict bound on the positions in voxels
    fn from_sizes_and_voxels<IVoxels>(
        sizes: Sizes<Self::IndexType>,
        voxels: IVoxels,
    ) -> Result<Self, InvalidVoxelWorldError<Self::IndexType>>
    where
        Self: Sized,
        IVoxels: IntoIterator<Item = PosVoxel<Self::IndexType>>,
    {
        let world = Self::partial_from_sizes_and_voxels(sizes, voxels)?;
        check_voxel_world(&world)?;
        Ok(world)
    }
    #[allow(clippy::type_complexity)]
    fn from_voxels<IVoxels>(
        voxels: IVoxels,
    ) -> Result<(Self, Pos<Self::IndexType>), InvalidVoxelWorldError<Self::IndexType>>
    where
        Self: Sized,
        IVoxels: IntoIterator<Item = PosVoxel<Self::IndexType>> + Clone,
    {
        let min_hull = minimal_pos_hull(voxels.clone().into_iter().map(|(pos, _)| pos));
        let (min_pos, max_pos_bound) = min_hull.start_end();

        let sizes = Sizes::new(max_pos_bound - min_pos);
        let voxels = voxels
            .into_iter()
            .map(|(pos, voxel)| (pos - min_pos, voxel));
        Self::from_sizes_and_voxels(sizes, voxels).map(|world| (world, min_pos))
    }

    /// It must hold that `self.size_ranges()` == `SizeRanges::from_sizes(self.sizes())`
    fn sizes(&self) -> Sizes<Self::IndexType>;

    /// Panics if `pos` is out of bounds (`0..sizes()`)
    fn set_voxel(&mut self, pos: Pos<Self::IndexType>, voxel: Voxel) -> Option<Voxel>;
}
