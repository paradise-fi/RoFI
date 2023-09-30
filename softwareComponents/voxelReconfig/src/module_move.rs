use crate::atoms;
use crate::module_repr::{get_other_body, is_module_repr};
use crate::pos::Pos;
use crate::voxel::{JointPosition, PosVoxel, Voxel};
use crate::voxel_world::{check_voxel_world, rotate_voxel};
use crate::voxel_world::{CenteredVoxelWorld, RotatedVoxelWorld, VoxelSubworld};
use crate::voxel_world::{NormVoxelWorld, VoxelWorld};
use enum_iterator::Sequence;
use itertools::Itertools;
use modular_bitfield::prelude::*;
use static_assertions::const_assert_eq;
use std::assert_matches::debug_assert_matches;

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

    pub fn is_possible(self, body_a: Voxel, body_b: Voxel) -> bool {
        let body = match self.joint() {
            Joint::Gamma => return true,
            Joint::Alpha => body_a,
            Joint::Beta => body_b,
        };
        body.joint_pos().rotated(self.angle()).is_some()
    }

    pub fn all_possible_moves(body_a: Voxel, body_b: Voxel) -> impl Iterator<Item = Move> {
        Self::iter_all().filter(move |module_move| module_move.is_possible(body_a, body_b))
    }

    fn get_rotation_center<TIndex: num::Num>(
        self,
        a_pos: Pos<TIndex>,
        b_pos: Pos<TIndex>,
    ) -> Pos<TIndex> {
        match self.joint() {
            Joint::Alpha | Joint::Gamma => a_pos,
            Joint::Beta => b_pos,
        }
    }

    fn get_alpha_rotation(rot_angle: atoms::RotationAngle, body_a: Voxel) -> atoms::Rotation {
        assert!(body_a.joint_pos().rotated(rot_angle).is_some());
        match body_a.joint_pos() {
            JointPosition::Zero => {
                let first_rot =
                    atoms::Rotation::new_with(body_a.x_conns_axis(), atoms::RotationAngle::Plus90);
                let rot = if first_rot.rotate_dir(body_a.body_dir()).is_positive()
                    == rot_angle.is_positive()
                {
                    first_rot
                } else {
                    atoms::Rotation::new_with(body_a.x_conns_axis(), atoms::RotationAngle::Minus90)
                };
                debug_assert_eq!(
                    rot.rotate_dir(body_a.body_dir()).is_positive(),
                    rot_angle.is_positive()
                );
                rot
            }
            JointPosition::Plus90 | JointPosition::Minus90 => {
                debug_assert_ne!(body_a.body_dir().axis(), body_a.z_conn_dir().axis());
                atoms::Rotation::new_from_to(body_a.body_dir(), body_a.z_conn_dir().opposite())
            }
        }
    }

    fn get_beta_rotation(rot_angle: atoms::RotationAngle, body_b: Voxel) -> atoms::Rotation {
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
                debug_assert_ne!(body_b.z_conn_dir().axis(), body_b.body_dir().axis());
                atoms::Rotation::new_from_to(body_b.z_conn_dir(), body_b.body_dir().opposite())
            }
        }
    }

    pub fn get_rotation(self, body_a: Voxel, body_b: Voxel) -> atoms::Rotation {
        assert!(self.is_possible(body_a, body_b));
        match self.joint() {
            Joint::Alpha => Self::get_alpha_rotation(self.angle(), body_a),
            Joint::Beta => Self::get_beta_rotation(self.angle(), body_b),
            Joint::Gamma => atoms::Rotation::new_from_dir(body_a.body_dir(), self.angle()),
        }
    }

    /// Expects that subworld from ShoeB was rotated
    fn apply_to_rotation_center<TWorld: NormVoxelWorld>(
        self,
        world: &mut TWorld,
        new_rot_center: Pos<TWorld::IndexType>,
        orig_rot_voxel: Voxel,
        rotation: atoms::Rotation,
    ) {
        match self.joint() {
            Joint::Gamma => {
                debug_assert_eq!(world.get_voxel(new_rot_center), Some(orig_rot_voxel));
            }
            Joint::Alpha => {
                let new_joint_pos = match orig_rot_voxel.joint_pos() {
                    JointPosition::Zero => {
                        if orig_rot_voxel.z_conn_dir().is_positive() {
                            JointPosition::Plus90
                        } else {
                            JointPosition::Minus90
                        }
                    }
                    JointPosition::Plus90 | JointPosition::Minus90 => JointPosition::Zero,
                };
                let old_value = world.set_voxel(
                    new_rot_center,
                    rotate_voxel(orig_rot_voxel, rotation).with_joint_pos(new_joint_pos),
                );
                debug_assert_eq!(old_value, Some(orig_rot_voxel));
            }
            Joint::Beta => {
                let new_joint_pos = match orig_rot_voxel.joint_pos() {
                    JointPosition::Zero => match self.angle() {
                        atoms::RotationAngle::Plus90 => JointPosition::Plus90,
                        atoms::RotationAngle::Minus90 => JointPosition::Minus90,
                    },
                    JointPosition::Plus90 | JointPosition::Minus90 => JointPosition::Zero,
                };
                let old_value =
                    world.set_voxel(new_rot_center, orig_rot_voxel.with_joint_pos(new_joint_pos));
                debug_assert_eq!(old_value, Some(rotate_voxel(orig_rot_voxel, rotation)));
            }
        }
    }

    pub fn apply<RWorld, TWorld>(
        self,
        module: PosVoxel<TWorld::IndexType>,
        split: &VoxelSubworld<TWorld>,
    ) -> Option<RWorld>
    where
        TWorld: VoxelWorld,
        RWorld: NormVoxelWorld<IndexType = TWorld::IndexType>,
        TWorld::IndexType: num::Integer,
    {
        assert!(is_module_repr(module.1));
        assert_eq!(split.get_voxel(module.0), Some(module.1));

        let complement_split = split.complement();

        let other_body = get_other_body(module, &complement_split)
            .expect("Second split doesn't contain body B (before move)");

        assert!(
            self.is_possible(module.1, other_body.1),
            "Invalid move {self:?}"
        );

        let rot_center = self.get_rotation_center(module.0, other_body.0);
        let rotation = self.get_rotation(module.1, other_body.1);
        let orig_rot_voxel = split
            .underlying_world()
            .get_voxel(rot_center)
            .expect("Underlying world has to contain rotation center");

        let first = CenteredVoxelWorld::<VoxelSubworld<_>, _>::new(split, rot_center);
        let second = CenteredVoxelWorld::new(complement_split, rot_center);
        let second = RotatedVoxelWorld::new(second, rotation);

        let (mut result, new_rot_center) = second.combine_with(&first)?;
        self.apply_to_rotation_center(&mut result, new_rot_center, orig_rot_voxel, rotation);

        debug_assert_matches!(check_voxel_world(&result), Ok(_));
        Some(result)
    }
}
