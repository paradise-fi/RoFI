use crate::pos::ord::OrdPos;
use crate::pos::{Pos, SizeRanges, Sizes};
use crate::voxel::{PosVoxel, Voxel};
use crate::voxel_world::{check_pos, debug_fmt_voxels, InvalidVoxelWorldError};
use crate::voxel_world::{NormVoxelWorld, VoxelWorld};
use litemap::LiteMap;

pub trait SortvecVoxelWorldIndex:
    num::Signed
    + Ord
    + Copy
    + std::hash::Hash
    + std::fmt::Debug
    + std::convert::TryInto<usize>
    + std::convert::TryFrom<usize>
{
}
impl<T> SortvecVoxelWorldIndex for T where
    Self: num::Signed
        + Ord
        + Copy
        + std::hash::Hash
        + std::fmt::Debug
        + std::convert::TryInto<usize>
        + std::convert::TryFrom<usize>
{
}

#[derive(Clone, PartialEq, Eq, Hash)]
pub struct SortvecVoxelWorld<IndexType: SortvecVoxelWorldIndex> {
    data: LiteMap<OrdPos<IndexType>, Voxel>,
    sizes: Sizes<IndexType>,
}

impl<TIndex: SortvecVoxelWorldIndex> std::fmt::Debug for SortvecVoxelWorld<TIndex> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("SortvecVoxelWorld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!("{indent}sizes: {:?},{ws_sep}", self.sizes()))?;

        f.write_fmt(format_args!("{indent}voxels: "))?;
        debug_fmt_voxels(self.all_voxels(), f, ws_sep, indent, indent, f.alternate())?;

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<TIndex: SortvecVoxelWorldIndex> VoxelWorld for SortvecVoxelWorld<TIndex> {
    type IndexType = TIndex;
    type PosVoxelIter<'a> = impl Iterator<Item = PosVoxel<Self::IndexType>> where Self: 'a;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
        SizeRanges::from_sizes(self.sizes())
    }

    fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
        self.data.iter().map(|(&OrdPos(pos), &voxel)| (pos, voxel))
    }

    fn get_voxel(&self, pos: Pos<Self::IndexType>) -> Option<Voxel> {
        self.data.get(&OrdPos(pos)).copied()
    }
}

impl<TIndex: SortvecVoxelWorldIndex> NormVoxelWorld for SortvecVoxelWorld<TIndex> {
    fn partial_from_sizes_and_voxels<IVoxels>(
        sizes: Sizes<Self::IndexType>,
        voxels: IVoxels,
    ) -> Result<Self, InvalidVoxelWorldError<Self::IndexType>>
    where
        Self: Sized,
        IVoxels: IntoIterator<Item = PosVoxel<Self::IndexType>>,
    {
        let mut data = LiteMap::new();
        for (pos, voxel) in voxels {
            check_pos(pos, SizeRanges::from_sizes(sizes))?;
            let old_data = data.insert(OrdPos(pos), voxel);
            if old_data.is_some() {
                return Err(InvalidVoxelWorldError::DuplicateVoxels(pos));
            }
        }

        Ok(Self { data, sizes })
    }

    fn sizes(&self) -> Sizes<Self::IndexType> {
        self.sizes
    }

    fn set_voxel(&mut self, pos: Pos<Self::IndexType>, voxel: Voxel) -> Option<Voxel> {
        check_pos(pos, self.size_ranges()).expect("Invalid position to set voxel");
        self.data.insert(OrdPos(pos), voxel)
    }
}

#[cfg(test)]
mod test {
    use crate::atoms;
    use crate::voxel::{JointPosition, Voxel};
    use crate::voxel_world::{normalized_eq_worlds, NormVoxelWorld};
    use std::collections::HashSet;

    type SortvecVoxelWorld = super::SortvecVoxelWorld<i8>;

    #[test]
    fn test_normalized_eq_single_def_module() {
        let (first_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Z, true),
                    false,
                    JointPosition::Zero,
                ),
            ),
            (
                [0, 0, 1].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Z, false),
                    false,
                    JointPosition::Zero,
                ),
            ),
        ])
        .unwrap();

        let (second_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    true,
                    JointPosition::Zero,
                ),
            ),
            (
                [1, 0, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    true,
                    JointPosition::Zero,
                ),
            ),
        ])
        .unwrap();

        assert_eq!(
            normalized_eq_worlds(&first_world).collect::<HashSet<_>>(),
            normalized_eq_worlds(&second_world).collect()
        );
    }

    #[test]
    fn test_normalized_eq_single_rot_module() {
        let (first_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, true),
                    false,
                    JointPosition::Zero,
                ),
            ),
            (
                [0, 1, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, false),
                    true,
                    JointPosition::Zero,
                ),
            ),
        ])
        .unwrap();

        let (second_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    false,
                    JointPosition::Zero,
                ),
            ),
            (
                [1, 0, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    true,
                    JointPosition::Zero,
                ),
            ),
        ])
        .unwrap();

        assert_eq!(
            normalized_eq_worlds(&first_world).collect::<HashSet<_>>(),
            normalized_eq_worlds(&second_world).collect()
        );
    }

    #[test]
    fn test_normalized_eq_single_jrot_module() {
        let (first_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [0, 1, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, false),
                    true,
                    JointPosition::Minus90,
                ),
            ),
        ])
        .unwrap();

        let (second_world, _) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    false,
                    JointPosition::Minus90,
                ),
            ),
            (
                [1, 0, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    false,
                    JointPosition::Plus90,
                ),
            ),
        ])
        .unwrap();

        assert_eq!(
            normalized_eq_worlds(&first_world).collect::<HashSet<_>>(),
            normalized_eq_worlds(&second_world).collect()
        );
    }

    #[test]
    fn test_normalized_single_module() {
        let (world, _pos) = SortvecVoxelWorld::from_voxels([
            (
                [0; 3].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
            ),
            (
                [1, 0, 0].into(),
                Voxel::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    false,
                    JointPosition::Minus90,
                ),
            ),
        ])
        .unwrap();

        println!(
            "{:?}",
            normalized_eq_worlds(&world)
                .map(|w| crate::serde::VoxelWorld::from_world(&w))
                .collect::<Vec<_>>()
        );

        assert_eq!(
            normalized_eq_worlds(&world).collect::<HashSet<_>>(),
            amplify::set![
                SortvecVoxelWorld::from_voxels([
                    (
                        [0, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, true),
                            true,
                            JointPosition::Plus90
                        ),
                    ),
                    (
                        [1, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, false),
                            false,
                            JointPosition::Minus90
                        ),
                    ),
                ])
                .unwrap()
                .0,
                SortvecVoxelWorld::from_voxels([
                    (
                        [0, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, true),
                            false,
                            JointPosition::Plus90
                        ),
                    ),
                    (
                        [1, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, false),
                            true,
                            JointPosition::Plus90
                        ),
                    ),
                ])
                .unwrap()
                .0,
                SortvecVoxelWorld::from_voxels([
                    (
                        [0, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, true),
                            false,
                            JointPosition::Minus90
                        ),
                    ),
                    (
                        [1, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, false),
                            true,
                            JointPosition::Minus90
                        ),
                    ),
                ])
                .unwrap()
                .0,
                SortvecVoxelWorld::from_voxels([
                    (
                        [0, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, true),
                            true,
                            JointPosition::Minus90
                        ),
                    ),
                    (
                        [1, 0, 0].into(),
                        Voxel::new_with(
                            atoms::Direction::new_with(atoms::Axis::X, false),
                            false,
                            JointPosition::Plus90
                        ),
                    ),
                ])
                .unwrap()
                .0
            ]
        )
    }
}
