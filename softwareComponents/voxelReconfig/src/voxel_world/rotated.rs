use super::{InvalidVoxelWorldError, NormVoxelWorld, VoxelWorld};
use crate::atoms;
use crate::pos::{Pos, SizeRanges, Sizes};
use crate::voxel::{JointPosition, PosVoxel, Voxel};
use iter_fixed::IntoIteratorFixed;

pub fn rotate_voxel(voxel: Voxel, rotation: atoms::Rotation) -> Voxel {
    Voxel::new_with(
        rotation.rotate_dir(voxel.body_dir()),
        !voxel.shoe_rotated(),
        match voxel.joint_pos() {
            JointPosition::Zero => JointPosition::Zero,
            JointPosition::Plus90 | JointPosition::Minus90 => {
                if rotation.rotate_dir(voxel.z_conn_dir()).is_positive() {
                    JointPosition::Plus90
                } else {
                    JointPosition::Minus90
                }
            }
        },
    )
}

fn combine_size_ranges<TIndex: num::Num + Ord>(
    lhs: SizeRanges<TIndex>,
    rhs: SizeRanges<TIndex>,
) -> SizeRanges<TIndex> {
    let (lhs_start, lhs_end) = lhs.start_end();
    let (rhs_start, rhs_end) = rhs.start_end();

    SizeRanges::new(
        lhs_start
            .as_array()
            .into_iter_fixed()
            .zip(rhs_start.as_array())
            .map(|(lhs, rhs)| std::cmp::min(lhs, rhs))
            .collect(),
        lhs_end
            .as_array()
            .into_iter_fixed()
            .zip(rhs_end.as_array())
            .map(|(lhs, rhs)| std::cmp::max(lhs, rhs))
            .collect(),
    )
}

pub struct RotatedVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    world: TWorldRef,
    rotation: atoms::Rotation,
    __phantom: std::marker::PhantomData<TWorld>,
}

impl<TWorld, TWorldRef> std::fmt::Debug for RotatedVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
    TWorld::IndexType: num::Integer,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("RotatedVoxelWorld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!(
            "{indent}rotation: {:?},{ws_sep}",
            self.rotation
        ))?;

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

impl<TWorld, TWorldRef> RotatedVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
{
    pub fn new(world: TWorldRef, rotation: atoms::Rotation) -> Self {
        Self {
            world,
            rotation,
            __phantom: Default::default(),
        }
    }

    fn world(&self) -> &TWorld {
        self.world.borrow()
    }

    fn get_new_pos(&self, orig_pos: Pos<TWorld::IndexType>) -> Pos<TWorld::IndexType> {
        self.rotation.rotate(orig_pos.as_array()).into()
    }
    fn get_orig_pos(&self, new_pos: Pos<TWorld::IndexType>) -> Pos<TWorld::IndexType> {
        self.rotation.inverse().rotate(new_pos.as_array()).into()
    }

    // The returned world is probably not valid (the rotation center is not updated)
    // but the rest should be valid (returns None if collision occured)
    pub fn combine_with<RWorld, UWorld>(
        &self,
        other: &UWorld,
    ) -> Option<(RWorld, Pos<TWorld::IndexType>)>
    where
        RWorld: NormVoxelWorld<IndexType = TWorld::IndexType>,
        UWorld: VoxelWorld<IndexType = TWorld::IndexType>,
        TWorld::IndexType: num::Integer,
    {
        let (min_pos, max_pos_bound) =
            combine_size_ranges(self.size_ranges(), other.size_ranges()).start_end();

        assert!(min_pos.as_array().iter().all(|pos| pos <= &num::zero()));
        assert!(max_pos_bound
            .as_array()
            .iter()
            .all(|pos| pos > &num::zero()));

        let sizes = Sizes::new(max_pos_bound - min_pos);

        let center = -min_pos;
        let voxels = self
            .all_voxels()
            .chain(other.all_voxels())
            .map(|(pos, voxel)| (pos + center, voxel));

        match RWorld::partial_from_sizes_and_voxels(sizes, voxels) {
            Ok(world) => Some((world, center)),
            Err(InvalidVoxelWorldError::DuplicateVoxels(_)) => None,
            Err(err) => Err(err).expect("Combining failed not by duplicates"),
        }
    }
}

impl<TWorld, TWorldRef> VoxelWorld for RotatedVoxelWorld<TWorld, TWorldRef>
where
    TWorld: VoxelWorld,
    TWorldRef: std::borrow::Borrow<TWorld>,
    TWorld::IndexType: num::Integer,
{
    type IndexType = TWorld::IndexType;
    type PosVoxelIter<'a> = impl Iterator<Item = PosVoxel<Self::IndexType>> where Self: 'a;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
        let size_ranges = self.world().size_ranges().as_ranges_array();
        SizeRanges::from_ranges_array(
            self.rotation
                .rotate(size_ranges.map(atoms::NegableRange))
                .map(From::from),
        )
    }

    fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
        Box::new(self.world().all_voxels().map(|(orig_pos, voxel)| {
            let rot_voxel = rotate_voxel(voxel, self.rotation);
            let rot_pos = self.get_new_pos(orig_pos);
            (rot_pos, rot_voxel)
        }))
    }

    fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel> {
        self.world()
            .get_voxel(self.get_orig_pos(pos))
            .map(|voxel| rotate_voxel(voxel, self.rotation))
    }
}
