use crate::atoms;
use crate::module_repr::is_module_repr;
use crate::pos::VoxelPos;
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::{Voxel, VoxelBody, VoxelBodyWithPos};
use crate::voxel_world::rotate_body;
use crate::voxel_world::{CenteredVoxelWorld, RotatedVoxelWorld, VoxelSubworld, VoxelWorld};
use enum_iterator::Sequence;
use itertools::Itertools;
use modular_bitfield::prelude::*;
use static_assertions::const_assert_eq;
use std::assert_matches::assert_matches;

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

    fn get_rotation(self, body_a: VoxelBody, body_b: VoxelBody) -> atoms::Rotation {
        let get_shoe_rotation = |shoe_body: VoxelBody| {
            let axis = shoe_body.x_conns_axis();
            if shoe_body.is_shoe_rotated() {
                atoms::Rotation::new_with(axis, self.angle().opposite())
            } else {
                atoms::Rotation::new_with(axis, self.angle())
            }
        };
        match self.joint() {
            Joint::Alpha => get_shoe_rotation(body_a),
            Joint::Beta => get_shoe_rotation(body_b),
            Joint::Gamma => atoms::Rotation::new_from_dir(body_a.other_body_dir(), self.angle()),
        }
    }

    /// Expects that subworld from ShoeB was rotated
    fn apply_to_rotation_center(self, world: &mut VoxelWorld, rot_center: VoxelPos) {
        if self.joint() == Joint::Gamma {
            return;
        }

        let rot_center = world.get_voxel_mut(rot_center).unwrap();

        let rot_center_body = rot_center
            .get_body()
            .expect("Rotation center cannot be empty");

        let orig_rotation = {
            let axis = rot_center_body.x_conns_axis();
            if rot_center_body.is_shoe_rotated() {
                atoms::Rotation::new_with(axis, self.angle().opposite())
            } else {
                atoms::Rotation::new_with(axis, self.angle())
            }
        };

        let rot_center_body = rotate_body(rot_center_body, orig_rotation);
        let new_joint_pos = rot_center_body
            .joint_pos()
            .rotated(self.angle())
            .expect("Move not possible");
        let new_body = rot_center_body.with_joint_pos(new_joint_pos);
        *rot_center = Voxel::new_with_body(new_body);
    }

    pub fn apply(self, module: VoxelBodyWithPos, split: VoxelSubworld) -> VoxelWorld {
        let complement_split = split.complement();

        assert!(is_module_repr(module.0));
        let (body_a, body_a_pos) = module;
        let body_b_pos = get_neighbour_pos(module).unwrap();
        let body_b = complement_split
            .get_body(body_b_pos)
            .expect("Second split doesn't contain body B (before move)");

        assert!(self.is_possible(body_a, body_b), "Invalid move");

        let rot_center = self.get_rotation_center(body_a_pos, body_b_pos);
        let rotation = self.get_rotation(body_a, body_b);

        let [first, second] =
            [split, complement_split].map(|subworld| CenteredVoxelWorld::new(subworld, rot_center));
        let second = RotatedVoxelWorld::new(second, rotation);

        let (mut result, rot_center) = second.combine_with(&first);
        self.apply_to_rotation_center(&mut result, rot_center);

        assert_matches!(result.check_voxel_world(), Ok(()));
        result
    }
}
