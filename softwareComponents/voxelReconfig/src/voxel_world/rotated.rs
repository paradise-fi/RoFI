use super::{CenteredVoxelWorld, VoxelWorld};
use crate::atoms;
use crate::pos::IndexType;
use crate::pos::{RelativeIndexType, RelativeVoxelPos, VoxelPos};
use crate::voxel::body::JointPosition;
use crate::voxel::{Voxel, VoxelBody};

pub fn rotate_body(body: VoxelBody, rotation: atoms::Rotation) -> VoxelBody {
    VoxelBody::new_with(
        rotation.rotate_dir(body.other_body_dir()),
        !body.is_shoe_rotated(),
        match body.joint_pos() {
            JointPosition::Zero => JointPosition::Zero,
            JointPosition::Plus90 | JointPosition::Minus90 => {
                if rotation.rotate_dir(body.z_conn_dir()).is_positive() {
                    JointPosition::Plus90
                } else {
                    JointPosition::Minus90
                }
            }
        },
    )
}

pub fn rotate_voxel(voxel: Voxel, rotation: atoms::Rotation) -> Voxel {
    match voxel.get_body() {
        Some(body) => Voxel::new_with_body(rotate_body(body, rotation)),
        None => Voxel::EMPTY,
    }
}

pub struct RotatedVoxelWorld<'a> {
    rotation: atoms::Rotation,
    world: CenteredVoxelWorld<'a>,
}

impl<'a> std::fmt::Debug for RotatedVoxelWorld<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let indent = if f.alternate() { "    " } else { "" };
        let ws_sep = if f.alternate() { "\n" } else { " " };
        f.write_str("RotatedVoxelWorld {")?;
        f.write_str(ws_sep)?;

        f.write_fmt(format_args!(
            "{indent}center: {:?},{ws_sep}",
            self.world.center()
        ))?;

        f.write_fmt(format_args!(
            "{indent}rotation: {:?},{ws_sep}",
            self.rotation
        ))?;

        f.write_fmt(format_args!(
            "{indent}sizes: {:?},{ws_sep}",
            self.size_ranges()
        ))?;

        f.write_fmt(format_args!("{indent}bodies: "))?;
        super::debug_fmt_bodies(self.all_bodies(), f, ws_sep, indent, indent, f.alternate())?;

        if f.alternate() {
            f.write_str(",")?;
        }
        f.write_str(ws_sep)?;
        f.write_str("}")
    }
}

impl<'a> RotatedVoxelWorld<'a> {
    pub fn new(world: CenteredVoxelWorld<'a>, rotation: atoms::Rotation) -> Self {
        Self { rotation, world }
    }

    pub fn size_ranges(&self) -> [std::ops::Range<RelativeIndexType>; 3] {
        let size_ranges = self.world.size_ranges();
        self.rotation
            .rotate(size_ranges.map(atoms::NegableRange))
            .map(From::from)
    }

    fn get_rel_pos(&self, rot_pos: RelativeVoxelPos) -> RelativeVoxelPos {
        let RelativeVoxelPos(rot_pos) = rot_pos;
        RelativeVoxelPos(self.rotation.inverse().rotate(rot_pos))
    }
    fn get_rot_pos(&self, rel_pos: RelativeVoxelPos) -> RelativeVoxelPos {
        let RelativeVoxelPos(rel_pos) = rel_pos;
        RelativeVoxelPos(self.rotation.rotate(rel_pos))
    }

    pub fn get_voxel(&self, pos: RelativeVoxelPos) -> Option<Voxel> {
        self.world
            .get_voxel(self.get_rel_pos(pos))
            .map(|voxel| rotate_voxel(voxel, self.rotation))
    }

    pub fn get_body(&self, pos: RelativeVoxelPos) -> Option<VoxelBody> {
        self.get_voxel(pos)?.get_body()
    }

    pub fn all_bodies(&self) -> impl Iterator<Item = (VoxelBody, RelativeVoxelPos)> + '_ {
        self.world.all_bodies().map(|(body, rel_pos)| {
            let rot_body = rotate_body(body, self.rotation);
            let rot_pos = self.get_rot_pos(rel_pos);
            (rot_body, rot_pos)
        })
    }

    pub fn combine_with(&self, other: &CenteredVoxelWorld) -> (VoxelWorld, VoxelPos) {
        let self_size_ranges = self.size_ranges();
        let other_size_ranges = other.size_ranges();

        let size_ranges = self_size_ranges.zip(other_size_ranges).map(|(lsr, rsr)| {
            assert!(
                lsr.contains(&0) || rsr.contains(&0),
                "one world has to contain the rotation center"
            );
            std::cmp::min(lsr.start, rsr.start)..std::cmp::max(lsr.end, rsr.end)
        });

        let sizes = VoxelPos(
            size_ranges
                .clone()
                .map(|r| IndexType::try_from(r.end - r.start).unwrap()),
        );
        let center =
            VoxelPos(size_ranges.map(|r| {
                IndexType::try_from(-r.start).expect("start has to be always non-positive")
            }));

        let bodies = self
            .all_bodies()
            .chain(other.all_bodies())
            .map(|(body, pos)| (body, pos.to_abs_pos(center).unwrap()));

        let world = VoxelWorld::from_sizes_and_bodies_unchecked(sizes, bodies).unwrap();
        (world, center)
    }
}
