use crate::atoms;
use crate::pos::{IndexType, VoxelPos};
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::{Voxel, VoxelBody, VoxelBodyWithPos, VoxelWithPos};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct VoxelWorld(atoms::Vec3D<Voxel>);

#[derive(Debug, Clone, Copy, thiserror::Error)]
pub enum InvalidVoxelWorldError {
    #[error("missing other body at {:?}", { let Self::MissingOtherBody((_, pos)) = self; pos })]
    MissingOtherBody(VoxelBodyWithPos),
}

impl VoxelWorld {
    pub fn new(sizes: VoxelPos) -> Self {
        let VoxelPos(sizes) = sizes;
        Self(atoms::Vec3D::new(sizes.map(Into::into)))
    }

    pub fn from_vec3d(
        vec: atoms::Vec3D<Voxel>,
    ) -> Result<Self, (atoms::Vec3D<Voxel>, InvalidVoxelWorldError)> {
        let world = Self(vec);
        match world.check_voxel_world() {
            Ok(()) => Ok(world),
            Err(err) => Err((world.0, err)),
        }
    }

    pub fn check_voxel_world(&self) -> Result<(), InvalidVoxelWorldError> {
        for size in self.0.sizes() {
            assert!(size > 0 && size - 1 <= IndexType::MAX.into());
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

    // Can return a world that is not valid
    pub fn filtered<P>(&self, predicate: P) -> Self
    where
        P: Fn(VoxelBodyWithPos) -> bool,
    {
        let predicate = &predicate;
        let VoxelWorld(world) = self;

        let mut new_world = world.clone();
        let [x_size, y_size, z_size] = world.sizes();
        for x in 0..x_size {
            for y in 0..y_size {
                for z in 0..z_size {
                    let indices = [x, y, z];
                    *new_world.get_mut(indices).unwrap() =
                        Voxel::from(world.get(indices).unwrap().get_body().filter(|&body| {
                            let voxel_pos = VoxelPos(indices.map(|i| u8::try_from(i).unwrap()));
                            predicate((body, voxel_pos))
                        }));
                }
            }
        }

        Self(new_world)
    }
}
