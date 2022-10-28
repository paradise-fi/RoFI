use crate::atoms;
use crate::pos::{compute_minimal_pos_hull, IndexType, VoxelPos};
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::{Voxel, VoxelBody, VoxelBodyWithPos, VoxelWithPos};

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
    ) -> Result<Self, InvalidVoxelWorldError> {
        let size_ranges = Self::compute_minimal_size_ranges(bodies.clone());

        let begin = size_ranges.clone().map(|size_range| size_range.start);
        let sizes = size_ranges.map(|size_range| size_range.end - size_range.start);

        Self::from_sizes_and_bodies(
            VoxelPos(sizes),
            bodies.into_iter().map(|(body, VoxelPos(pos))| {
                let pos = VoxelPos(pos.zip(begin).map(|(pos_i, begin_i)| pos_i - begin_i));
                (body, pos)
            }),
        )
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

    pub fn is_minimal_size(&self) -> bool {
        Self::compute_minimal_size_ranges(self.all_bodies()) == self.sizes().0.map(|size| 0..size)
    }

    pub fn dump_bodies(&self) {
        println!("VoxelWorld {{ bodies=[");
        for (body, pos) in self.all_bodies() {
            println!("    {:?}: {:?},", pos, body);
        }
        println!("] }}");
    }
}
