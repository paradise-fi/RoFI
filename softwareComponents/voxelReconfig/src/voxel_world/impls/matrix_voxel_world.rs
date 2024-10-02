use crate::atoms;
use crate::pos::{Pos, SizeRanges, Sizes};
use crate::voxel::opt::VoxelOpt;
use crate::voxel::{PosVoxel, Voxel};
use crate::voxel_world::{check_pos, debug_fmt_voxels, InvalidVoxelWorldError};
use crate::voxel_world::{NormVoxelWorld, VoxelWorld};

pub trait MatrixVoxelWorldIndex:
    num::Signed
    + Ord
    + Copy
    + std::hash::Hash
    + std::fmt::Debug
    + std::convert::TryInto<usize>
    + std::convert::TryFrom<usize>
{
}
impl<T> MatrixVoxelWorldIndex for T where
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
pub struct MatrixVoxelWorld<IndexType: MatrixVoxelWorldIndex> {
    data: atoms::Vec3D<VoxelOpt>,
    __phantom: std::marker::PhantomData<IndexType>,
}

impl<TIndex: MatrixVoxelWorldIndex> std::fmt::Debug for MatrixVoxelWorld<TIndex> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("MatrixVoxelWorld {")?;
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

impl<TIndex: MatrixVoxelWorldIndex> MatrixVoxelWorld<TIndex> {
    pub fn to_inner_idx(position: Pos<TIndex>) -> Result<[usize; 3], String> {
        let result = position.as_array().map(|pos| pos.try_into().ok());
        if result.iter().all(Option::is_some) {
            Ok(result.map(Option::unwrap))
        } else {
            Err(format!("Cannot convert {position:?} to usize position"))
        }
    }
    pub fn to_position(inner_idx: [usize; 3]) -> Result<Pos<TIndex>, String> {
        let result = inner_idx.map(|idx| TIndex::try_from(idx).ok());
        if result.iter().all(Option::is_some) {
            Ok(result.map(Option::unwrap).into())
        } else {
            Err(format!(
                "Cannot convert {inner_idx:?} to IndexType position"
            ))
        }
    }
}

impl<TIndex: MatrixVoxelWorldIndex> VoxelWorld for MatrixVoxelWorld<TIndex> {
    type IndexType = TIndex;
    type PosVoxelIter<'a> = impl Iterator<Item = PosVoxel<Self::IndexType>> where Self: 'a;

    fn size_ranges(&self) -> SizeRanges<Self::IndexType> {
        SizeRanges::from_sizes(self.sizes())
    }

    fn all_voxels(&self) -> Self::PosVoxelIter<'_> {
        self.data
            .get_data()
            .zip(0..)
            .flat_map(|(plain, x)| {
                plain.zip(0..).flat_map(move |(row, y)| {
                    row.iter()
                        .copied()
                        .zip(0..)
                        .map(move |(voxel, z)| (Self::to_position([x, y, z]).unwrap(), voxel))
                })
            })
            .filter_map(|(pos, voxel)| Some((pos, voxel.get_voxel()?)))
    }

    fn get_voxel(&self, position: Pos<Self::IndexType>) -> Option<Voxel> {
        self.data
            .get(Self::to_inner_idx(position).ok()?)?
            .get_voxel()
    }
}

impl<TIndex: MatrixVoxelWorldIndex> NormVoxelWorld for MatrixVoxelWorld<TIndex> {
    fn partial_from_sizes_and_voxels<IVoxels>(
        sizes: Sizes<Self::IndexType>,
        voxels: IVoxels,
    ) -> Result<Self, InvalidVoxelWorldError<Self::IndexType>>
    where
        Self: Sized,
        IVoxels: IntoIterator<Item = PosVoxel<Self::IndexType>>,
    {
        let mut world = Self {
            data: atoms::Vec3D::new(
                Self::to_inner_idx(sizes.get()).expect("Given size doesn't fit"),
            ),
            __phantom: Default::default(),
        };

        for (pos, voxel) in voxels {
            check_pos(pos, SizeRanges::from_sizes(sizes))?;
            let old_value = world.set_voxel(pos, voxel);
            if old_value.is_some() {
                return Err(InvalidVoxelWorldError::DuplicateVoxels(pos));
            }
        }

        Ok(world)
    }

    fn sizes(&self) -> Sizes<Self::IndexType> {
        Sizes::new(Self::to_position(self.data.sizes()).unwrap())
    }

    fn set_voxel(&mut self, pos: Pos<Self::IndexType>, voxel: Voxel) -> Option<Voxel> {
        check_pos(pos, self.size_ranges()).expect("Invalid position to set voxel");
        let pos = Self::to_inner_idx(pos).unwrap();
        let voxel_mut = self.data.get_mut(pos).unwrap();
        std::mem::replace(voxel_mut, VoxelOpt::new_with(voxel)).get_voxel()
    }
}

#[cfg(test)]
mod test {
    use crate::atoms;
    use crate::voxel::{JointPosition, Voxel};
    use crate::voxel_world::{normalized_eq_worlds, NormVoxelWorld};
    use std::collections::HashSet;

    type MatrixVoxelWorld = super::MatrixVoxelWorld<i8>;

    #[test]
    fn test_normalized_eq_single_def_module() {
        let (first_world, _) = MatrixVoxelWorld::from_voxels([
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

        let (second_world, _) = MatrixVoxelWorld::from_voxels([
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
        let (first_world, _) = MatrixVoxelWorld::from_voxels([
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

        let (second_world, _) = MatrixVoxelWorld::from_voxels([
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
        let (first_world, _) = MatrixVoxelWorld::from_voxels([
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

        let (second_world, _) = MatrixVoxelWorld::from_voxels([
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
        let (world, _pos) = MatrixVoxelWorld::from_voxels([
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
                MatrixVoxelWorld::from_voxels([
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
                MatrixVoxelWorld::from_voxels([
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
                MatrixVoxelWorld::from_voxels([
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
                MatrixVoxelWorld::from_voxels([
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
