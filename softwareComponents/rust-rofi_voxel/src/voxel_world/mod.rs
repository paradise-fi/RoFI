mod centered;
mod rotated;
mod subworld;
mod world_rotation;

pub use centered::CenteredVoxelWorld;
pub use rotated::{rotate_body, rotate_voxel, RotatedVoxelWorld};
pub use subworld::VoxelSubworld;

use crate::pos::{compute_minimal_pos_hull, IndexType, RelativeIndexType, VoxelPos};
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::{Voxel, VoxelBody, VoxelBodyWithPos, VoxelWithPos};
use crate::{atoms, pos::RelativeVoxelPos};
use std::assert_matches::{assert_matches, debug_assert_matches};
use world_rotation::WorldRotation;

#[derive(Debug, Clone, amplify::Error)]
pub enum InvalidVoxelWorldError {
    MissingOtherBody(VoxelBodyWithPos),
    NotMinimalSize {
        current: VoxelPos,
        minimal: [std::ops::Range<IndexType>; 3],
    },
    VoxelOutOfBounds {
        pos: VoxelPos,
        sizes: VoxelPos,
    },
}
impl std::fmt::Display for InvalidVoxelWorldError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            InvalidVoxelWorldError::MissingOtherBody((_, pos)) => {
                write!(f, "Missing other body at {pos:?}")
            }
            InvalidVoxelWorldError::NotMinimalSize { current, minimal } => {
                write!(
                    f,
                    "Not minimal size (current: {current:?}, minimal: {minimal:?})"
                )
            }
            InvalidVoxelWorldError::VoxelOutOfBounds { pos, sizes } => {
                write!(
                    f,
                    "Voxel is out of bounds (voxel pos: {pos:?}, sizes: {sizes:?})"
                )
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct VoxelWorld(atoms::Vec3D<Voxel>);

impl VoxelWorld {
    fn from_sizes_and_bodies_unchecked<IBodies: IntoIterator<Item = VoxelBodyWithPos>>(
        sizes: VoxelPos,
        bodies: IBodies,
    ) -> Result<Self, InvalidVoxelWorldError> {
        let mut world = Self(atoms::Vec3D::new(sizes.0.map(Into::into)));

        for (body, pos) in bodies {
            let voxel_mut = world
                .get_voxel_mut(pos)
                .ok_or(InvalidVoxelWorldError::VoxelOutOfBounds { pos, sizes })?;
            *voxel_mut = crate::voxel::Voxel::new_with_body(body);
        }

        Ok(world)
    }

    pub fn from_sizes_and_bodies<IBodies: IntoIterator<Item = VoxelBodyWithPos>>(
        sizes: VoxelPos,
        bodies: IBodies,
    ) -> Result<Self, InvalidVoxelWorldError> {
        let world = Self::from_sizes_and_bodies_unchecked(sizes, bodies)?;
        world.check_voxel_world()?;
        Ok(world)
    }

    pub fn from_bodies<IBodies: IntoIterator<Item = VoxelBodyWithPos> + Clone>(
        bodies: IBodies,
    ) -> Result<(Self, VoxelPos), InvalidVoxelWorldError> {
        let size_ranges = Self::compute_minimal_size_ranges(bodies.clone());

        let begin = size_ranges.clone().map(|size_range| size_range.start);
        let sizes = size_ranges.map(|size_range| size_range.end - size_range.start);

        let world = Self::from_sizes_and_bodies(
            VoxelPos(sizes),
            bodies.into_iter().map(|(body, VoxelPos(pos))| {
                let pos = VoxelPos(pos.zip(begin).map(|(pos_i, begin_i)| pos_i - begin_i));
                (body, pos)
            }),
        )?;
        Ok((world, VoxelPos(begin)))
    }
    pub fn from_bodies_rel_pos<IBodies>(
        bodies: IBodies,
    ) -> Result<(Self, RelativeVoxelPos), InvalidVoxelWorldError>
    where
        IBodies: IntoIterator<Item = (VoxelBody, RelativeVoxelPos)> + Clone,
    {
        let size_ranges = Self::compute_minimal_size_ranges_rel_pos(bodies.clone());

        let begin = size_ranges.clone().map(|size_range| size_range.start);
        let sizes = size_ranges.map(|size_range| size_range.end - size_range.start);

        let world = Self::from_sizes_and_bodies(
            VoxelPos(sizes.map(|pos| IndexType::try_from(pos).unwrap())),
            bodies.into_iter().map(|(body, RelativeVoxelPos(pos))| {
                let pos = VoxelPos(
                    pos.zip(begin)
                        .map(|(pos_i, begin_i)| IndexType::try_from(pos_i - begin_i).unwrap()),
                );
                (body, pos)
            }),
        )?;

        Ok((world, RelativeVoxelPos(begin)))
    }

    pub fn check_voxel_world(&self) -> Result<(), InvalidVoxelWorldError> {
        for size in self.0.sizes() {
            assert!(size > 0 && size <= IndexType::MAX.into());
        }

        if !self.is_minimal_size() {
            return Err(InvalidVoxelWorldError::NotMinimalSize {
                current: self.sizes(),
                minimal: Self::compute_minimal_size_ranges(self.all_bodies()),
            });
        }

        for body_with_pos in self.all_bodies() {
            use InvalidVoxelWorldError::MissingOtherBody;

            let neighbour_pos =
                get_neighbour_pos(body_with_pos).map_err(|_| MissingOtherBody(body_with_pos))?;
            let neighbour = self
                .get_body(neighbour_pos)
                .ok_or(MissingOtherBody(body_with_pos))?;
            if get_neighbour_pos((neighbour, neighbour_pos)) != Ok(body_with_pos.1) {
                return Err(MissingOtherBody(body_with_pos));
            }
        }
        Ok(())
    }

    pub fn sizes(&self) -> VoxelPos {
        let Self(world) = self;
        VoxelPos(world.sizes().map(IndexType::try_from).map(Result::unwrap))
    }

    pub fn all_voxels(&self) -> impl Iterator<Item = VoxelWithPos> + '_ {
        self.0.get_data().zip(0..).flat_map(|(plain, x)| {
            plain.zip(0..).flat_map(move |(row, y)| {
                row.iter()
                    .copied()
                    .zip(0..)
                    .map(move |(voxel, z)| (voxel, VoxelPos([x, y, z])))
            })
        })
    }

    pub fn all_voxels_mut(&mut self) -> impl Iterator<Item = (&mut Voxel, VoxelPos)> {
        self.0.get_data_mut().zip(0..).flat_map(|(plain, x)| {
            plain.zip(0..).flat_map(move |(row, y)| {
                row.iter_mut()
                    .zip(0..)
                    .map(move |(voxel, z)| (voxel, VoxelPos([x, y, z])))
            })
        })
    }

    pub fn all_bodies(&self) -> impl Iterator<Item = VoxelBodyWithPos> + '_ {
        self.all_voxels()
            .filter_map(|(voxel, indices)| Some((voxel.get_body()?, indices)))
    }

    pub fn get_voxel(&self, position: VoxelPos) -> Option<Voxel> {
        let VoxelPos(indices) = position;
        self.0.get(indices.map(Into::into)).copied()
    }
    pub fn get_voxel_mut(&mut self, position: VoxelPos) -> Option<&mut Voxel> {
        let VoxelPos(indices) = position;
        self.0.get_mut(indices.map(Into::into))
    }

    pub fn get_body(&self, position: VoxelPos) -> Option<VoxelBody> {
        self.get_voxel(position)?.get_body()
    }

    pub fn compute_minimal_size_ranges<IBodies: IntoIterator<Item = VoxelBodyWithPos>>(
        bodies: IBodies,
    ) -> [std::ops::Range<IndexType>; 3] {
        compute_minimal_pos_hull(bodies.into_iter().map(|(_, VoxelPos(pos))| pos))
            .expect("No modules in VoxelWorld")
    }
    pub fn compute_minimal_size_ranges_rel_pos<IBodies>(
        bodies: IBodies,
    ) -> [std::ops::Range<RelativeIndexType>; 3]
    where
        IBodies: IntoIterator<Item = (VoxelBody, RelativeVoxelPos)>,
    {
        compute_minimal_pos_hull(bodies.into_iter().map(|(_, RelativeVoxelPos(pos))| pos))
            .expect("No modules in VoxelWorld")
    }

    pub fn is_minimal_size(&self) -> bool {
        Self::compute_minimal_size_ranges(self.all_bodies()) == self.sizes().0.map(|size| 0..size)
    }

    fn are_sizes_normalized(sizes: VoxelPos) -> bool {
        let VoxelPos([x_size, y_size, z_size]) = sizes;
        x_size >= y_size && y_size >= z_size
    }

    pub fn is_normalized(&self) -> bool {
        Self::are_sizes_normalized(self.sizes())
    }

    pub fn as_one_of_norm_eq_world(self) -> Self {
        if self.is_normalized() {
            self
        } else {
            self.normalized_eq_worlds()
                .next()
                .expect("There has to be a normalized version of world")
        }
    }

    // For sizes in normalized worlds it holds that size.x >= size.y >= size.z
    //
    // The iterator can return multiple equal worlds (in case the world is symmetrical)
    pub fn normalized_eq_worlds(&self) -> impl Iterator<Item = Self> + '_ {
        assert_matches!(Self::check_voxel_world(self), Ok(()));
        enum_iterator::all::<WorldRotation>().filter_map(move |world_rot| {
            let transformed_sizes = world_rot.rotate_sizes(self.sizes());
            if Self::are_sizes_normalized(transformed_sizes) {
                let transformed_world = world_rot.rotate_world(self);
                debug_assert_eq!(transformed_world.sizes(), transformed_sizes);
                debug_assert_matches!(Self::check_voxel_world(&transformed_world), Ok(()));
                debug_assert!(transformed_world.is_normalized());
                Some(transformed_world)
            } else {
                None
            }
        })
    }

    pub fn dump_bodies(&self) {
        println!("VoxelWorld {{ bodies=[");
        for (body, pos) in self.all_bodies() {
            println!("    {pos:?}: {body:?},");
        }
        println!("] }}");
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::atoms::{Axis, Direction};
    use crate::voxel::body::JointPosition;
    use std::collections::HashSet;

    #[test]
    fn test_normalized_eq_single_def_module() {
        let (first_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Z, true),
                    false,
                    JointPosition::Zero,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Z, false),
                    false,
                    JointPosition::Zero,
                ),
                VoxelPos([0, 0, 1]),
            ),
        ])
        .unwrap();

        let (second_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    true,
                    JointPosition::Zero,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    true,
                    JointPosition::Zero,
                ),
                VoxelPos([1, 0, 0]),
            ),
        ])
        .unwrap();

        assert_eq!(
            first_world.normalized_eq_worlds().collect::<HashSet<_>>(),
            second_world.normalized_eq_worlds().collect()
        );
    }

    #[test]
    fn test_normalized_eq_single_rot_module() {
        let (first_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, true),
                    false,
                    JointPosition::Zero,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, false),
                    true,
                    JointPosition::Zero,
                ),
                VoxelPos([0, 1, 0]),
            ),
        ])
        .unwrap();

        let (second_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    false,
                    JointPosition::Zero,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    true,
                    JointPosition::Zero,
                ),
                VoxelPos([1, 0, 0]),
            ),
        ])
        .unwrap();

        assert_eq!(
            first_world.normalized_eq_worlds().collect::<HashSet<_>>(),
            second_world.normalized_eq_worlds().collect()
        );
    }

    #[test]
    fn test_normalized_eq_single_jrot_module() {
        let (first_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, true),
                    true,
                    JointPosition::Plus90,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::Y, false),
                    true,
                    JointPosition::Minus90,
                ),
                VoxelPos([0, 1, 0]),
            ),
        ])
        .unwrap();

        let (second_world, _) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    false,
                    JointPosition::Minus90,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    false,
                    JointPosition::Plus90,
                ),
                VoxelPos([1, 0, 0]),
            ),
        ])
        .unwrap();

        assert_eq!(
            first_world.normalized_eq_worlds().collect::<HashSet<_>>(),
            second_world.normalized_eq_worlds().collect()
        );
    }

    #[test]
    fn test_normalized_single_module() {
        let (world, _pos) = VoxelWorld::from_bodies([
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, true),
                    true,
                    JointPosition::Plus90,
                ),
                VoxelPos([0; 3]),
            ),
            (
                VoxelBody::new_with(
                    atoms::Direction::new_with(atoms::Axis::X, false),
                    false,
                    JointPosition::Minus90,
                ),
                VoxelPos([1, 0, 0]),
            ),
        ])
        .unwrap();

        println!(
            "{:?}",
            world
                .normalized_eq_worlds()
                .map(|w| crate::serde::VoxelWorld::from_world(&w))
                .collect::<Vec<_>>()
        );

        assert_eq!(
            world.normalized_eq_worlds().collect::<HashSet<_>>(),
            amplify::set![
                VoxelWorld::from_bodies([
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, true),
                            true,
                            JointPosition::Plus90
                        ),
                        VoxelPos([0, 0, 0])
                    ),
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, false),
                            false,
                            JointPosition::Minus90
                        ),
                        VoxelPos([1, 0, 0])
                    ),
                ])
                .unwrap()
                .0,
                VoxelWorld::from_bodies([
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, true),
                            false,
                            JointPosition::Plus90
                        ),
                        VoxelPos([0, 0, 0])
                    ),
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, false),
                            true,
                            JointPosition::Plus90
                        ),
                        VoxelPos([1, 0, 0])
                    ),
                ])
                .unwrap()
                .0,
                VoxelWorld::from_bodies([
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, true),
                            false,
                            JointPosition::Minus90
                        ),
                        VoxelPos([0, 0, 0])
                    ),
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, false),
                            true,
                            JointPosition::Minus90
                        ),
                        VoxelPos([1, 0, 0])
                    ),
                ])
                .unwrap()
                .0,
                VoxelWorld::from_bodies([
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, true),
                            true,
                            JointPosition::Minus90
                        ),
                        VoxelPos([0, 0, 0])
                    ),
                    (
                        VoxelBody::new_with(
                            Direction::new_with(Axis::X, false),
                            false,
                            JointPosition::Plus90
                        ),
                        VoxelPos([1, 0, 0])
                    ),
                ])
                .unwrap()
                .0
            ]
        )
    }
}
