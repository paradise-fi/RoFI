use crate::atoms;
use crate::module_repr::is_module_repr;
use crate::pos::VoxelPos;
use crate::voxel::body::{get_neighbour_pos, JointPosition};
use crate::voxel::{Voxel, VoxelBody, VoxelBodyWithPos};
use crate::voxel_world::rotate_body;
use crate::voxel_world::{CenteredVoxelWorld, RotatedVoxelWorld, VoxelSubworld, VoxelWorld};
use enum_iterator::Sequence;
use itertools::Itertools;
use modular_bitfield::prelude::*;
use static_assertions::const_assert_eq;

#[derive(
    BitfieldSpecifier,
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    PartialOrd,
    Ord,
    Hash,
    Sequence,
    amplify::Display,
)]
#[display(Debug)]
#[repr(u8)]
#[bits = 2]
pub enum Joint {
    Alpha,
    Beta,
    Gamma,
}
const_assert_eq!(Joint::CARDINALITY, 3);

#[bitfield(bits = 3)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Move {
    pub joint: Joint,
    pub angle: atoms::RotationAngle,
}

impl Move {
    pub fn new_with(joint: Joint, angle: atoms::RotationAngle) -> Self {
        Self::new().with_joint(joint).with_angle(angle)
    }

    pub fn iter_all() -> impl Iterator<Item = Self> {
        enum_iterator::all()
            .cartesian_product(enum_iterator::all())
            .map(|(joint, angle)| Self::new_with(joint, angle))
    }

    pub fn is_possible(self, body_a: VoxelBody, body_b: VoxelBody) -> bool {
        let body = match self.joint() {
            Joint::Gamma => return true,
            Joint::Alpha => body_a,
            Joint::Beta => body_b,
        };
        body.joint_pos().rotated(self.angle()).is_some()
    }

    pub fn all_possible_moves(body_a: VoxelBody, body_b: VoxelBody) -> impl Iterator<Item = Move> {
        Self::iter_all().filter(move |module_move| module_move.is_possible(body_a, body_b))
    }

    fn get_rotation_center(self, body_a_pos: VoxelPos, body_b_pos: VoxelPos) -> VoxelPos {
        match self.joint() {
            Joint::Alpha | Joint::Gamma => body_a_pos,
            Joint::Beta => body_b_pos,
        }
    }

    fn get_alpha_rotation(rot_angle: atoms::RotationAngle, body_a: VoxelBody) -> atoms::Rotation {
        assert!(body_a.joint_pos().rotated(rot_angle).is_some());
        match body_a.joint_pos() {
            JointPosition::Zero => {
                let first_rot =
                    atoms::Rotation::new_with(body_a.x_conns_axis(), atoms::RotationAngle::Plus90);
                let rot = if first_rot.rotate_dir(body_a.other_body_dir()).is_positive()
                    == rot_angle.is_positive()
                {
                    first_rot
                } else {
                    atoms::Rotation::new_with(body_a.x_conns_axis(), atoms::RotationAngle::Minus90)
                };
                debug_assert_eq!(
                    rot.rotate_dir(body_a.other_body_dir()).is_positive(),
                    rot_angle.is_positive()
                );
                rot
            }
            JointPosition::Plus90 | JointPosition::Minus90 => {
                debug_assert_ne!(body_a.other_body_dir().axis(), body_a.z_conn_dir().axis());
                atoms::Rotation::new_from_to(
                    body_a.other_body_dir(),
                    body_a.z_conn_dir().opposite(),
                )
            }
        }
    }

    fn get_beta_rotation(rot_angle: atoms::RotationAngle, body_b: VoxelBody) -> atoms::Rotation {
        assert!(body_b.joint_pos().rotated(rot_angle).is_some());
        match body_b.joint_pos() {
            JointPosition::Zero => {
                let first_rot =
                    atoms::Rotation::new_with(body_b.x_conns_axis(), atoms::RotationAngle::Plus90);
                let rot = if first_rot.rotate_dir(body_b.z_conn_dir()).is_positive()
                    == rot_angle.is_positive()
                {
                    first_rot
                } else {
                    atoms::Rotation::new_with(body_b.x_conns_axis(), atoms::RotationAngle::Minus90)
                };
                debug_assert_eq!(
                    rot.rotate_dir(body_b.z_conn_dir()).is_positive(),
                    rot_angle.is_positive()
                );
                rot
            }
            JointPosition::Plus90 | JointPosition::Minus90 => {
                debug_assert_ne!(body_b.z_conn_dir().axis(), body_b.other_body_dir().axis());
                atoms::Rotation::new_from_to(
                    body_b.z_conn_dir(),
                    body_b.other_body_dir().opposite(),
                )
            }
        }
    }

    fn get_rotation(self, body_a: VoxelBody, body_b: VoxelBody) -> atoms::Rotation {
        assert!(self.is_possible(body_a, body_b));
        match self.joint() {
            Joint::Alpha => Self::get_alpha_rotation(self.angle(), body_a),
            Joint::Beta => Self::get_beta_rotation(self.angle(), body_b),
            Joint::Gamma => atoms::Rotation::new_from_dir(body_a.other_body_dir(), self.angle()),
        }
    }

    /// Expects that subworld from ShoeB was rotated
    fn apply_to_rotation_center(
        self,
        world: &mut VoxelWorld,
        new_rot_center: VoxelPos,
        orig_rot_body: VoxelBody,
        rotation: atoms::Rotation,
    ) {
        match self.joint() {
            Joint::Gamma => {}
            Joint::Alpha => {
                let rot_center = world.get_voxel_mut(new_rot_center).unwrap();
                assert!(rot_center.has_body());

                debug_assert_eq!(rot_center.get_body(), Some(orig_rot_body));

                let new_joint_pos = match orig_rot_body.joint_pos() {
                    JointPosition::Zero => {
                        if orig_rot_body.z_conn_dir().is_positive() {
                            JointPosition::Plus90
                        } else {
                            JointPosition::Minus90
                        }
                    }
                    JointPosition::Plus90 | JointPosition::Minus90 => JointPosition::Zero,
                };
                *rot_center = Voxel::new_with_body(
                    rotate_body(orig_rot_body, rotation).with_joint_pos(new_joint_pos),
                );
            }
            Joint::Beta => {
                let rot_center = world.get_voxel_mut(new_rot_center).unwrap();
                assert!(rot_center.has_body());

                debug_assert_eq!(
                    rot_center.get_body(),
                    Some(rotate_body(orig_rot_body, rotation))
                );

                let new_joint_pos = match orig_rot_body.joint_pos() {
                    JointPosition::Zero => match self.angle() {
                        atoms::RotationAngle::Plus90 => JointPosition::Plus90,
                        atoms::RotationAngle::Minus90 => JointPosition::Minus90,
                    },
                    JointPosition::Plus90 | JointPosition::Minus90 => JointPosition::Zero,
                };
                *rot_center = Voxel::new_with_body(orig_rot_body.with_joint_pos(new_joint_pos));
            }
        }
    }

    // Can return non-valid world (is some modules collide)
    pub fn apply(self, module: VoxelBodyWithPos, split: VoxelSubworld) -> VoxelWorld {
        let complement_split = split.complement();

        assert!(is_module_repr(module.0));
        assert_eq!(split.get_body(module.1), Some(module.0));
        let (body_a, body_a_pos) = module;
        let body_b_pos = get_neighbour_pos(module).unwrap();
        let body_b = complement_split
            .get_body(body_b_pos)
            .expect("Second split doesn't contain body B (before move)");

        assert!(self.is_possible(body_a, body_b), "Invalid move {self:?}");

        let rot_center = self.get_rotation_center(body_a_pos, body_b_pos);
        let rotation = self.get_rotation(body_a, body_b);
        let orig_rot_body = split.underlying_world().get_body(rot_center).unwrap();

        let [first, second] =
            [split, complement_split].map(|subworld| CenteredVoxelWorld::new(subworld, rot_center));
        let second = RotatedVoxelWorld::new(second, rotation);

        let (mut result, new_rot_center) = second.combine_with(&first);
        self.apply_to_rotation_center(&mut result, new_rot_center, orig_rot_body, rotation);

        result
    }
}
