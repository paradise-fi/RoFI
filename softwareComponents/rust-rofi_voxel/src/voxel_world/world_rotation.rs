use super::VoxelWorld;
use crate::pos::VoxelPos;
use crate::{atoms, voxel::VoxelBody};
use enum_iterator::Sequence;

#[derive(Debug, PartialEq, Eq, Sequence)]
pub struct WorldRotation {
    /// Axis at which rotated [1,0,0] has value 1
    x_rotates_to: atoms::Axis,
    /// Is axis negated before rotation
    neg_axis: [bool; 3],
}

impl WorldRotation {
    fn swaps_axes_order(&self) -> bool {
        self.neg_axis.iter().filter(|&is_neg| *is_neg).count() % 2 != 0
    }
    /// Returns [x_rotates_to, y_rotates_to, z_rotates_to]
    fn axes_rotate_to(&self) -> [atoms::Axis; 3] {
        if self.swaps_axes_order() {
            [
                self.x_rotates_to,
                self.x_rotates_to.prev_axis(),
                self.x_rotates_to.next_axis(),
            ]
        } else {
            [
                self.x_rotates_to,
                self.x_rotates_to.next_axis(),
                self.x_rotates_to.prev_axis(),
            ]
        }
    }

    fn rotated_pos_indices(&self) -> [usize; 3] {
        let axes_rotate_to = self.axes_rotate_to();
        let mut result = [None; 3];
        for (axis, i) in axes_rotate_to.into_iter().zip(0..) {
            debug_assert!(axis.as_index() < result.len());
            result[axis.as_index()] = Some(i);
        }
        debug_assert!(result.iter().all(|i| i.is_some()));
        debug_assert!(result.iter().all(|i: &Option<usize>| i.unwrap() < 3));
        result.map(Option::unwrap)
    }

    fn rotate_axis(&self, axis: atoms::Axis) -> atoms::Axis {
        debug_assert!(axis.as_index() < self.axes_rotate_to().len());
        self.axes_rotate_to()[axis.as_index()]
    }

    fn rotate_direction(&self, direction: atoms::Direction) -> atoms::Direction {
        debug_assert!(direction.axis().as_index() < self.neg_axis.len());
        let is_positive = direction.is_positive() ^ self.neg_axis[direction.axis().as_index()];
        atoms::Direction::new_with(self.rotate_axis(direction.axis()), is_positive)
    }

    pub fn rotate_sizes(&self, sizes: VoxelPos) -> VoxelPos {
        let VoxelPos(sizes) = sizes;
        VoxelPos(self.rotated_pos_indices().map(|pos_idx| sizes[pos_idx]))
    }

    fn rotate_pos(&self, pos: VoxelPos, orig_sizes: VoxelPos) -> VoxelPos {
        let VoxelPos(pos) = pos;
        let VoxelPos(sizes) = orig_sizes;
        let negated_pos = pos
            .zip(sizes)
            .zip(self.neg_axis)
            .map(
                |((pos_i, size_i), is_neg_i)| {
                    if is_neg_i {
                        size_i - 1 - pos_i
                    } else {
                        pos_i
                    }
                },
            );
        self.rotate_sizes(VoxelPos(negated_pos))
    }

    fn rotate_body(&self, body: VoxelBody) -> VoxelBody {
        let other_body_dir = self.rotate_direction(body.other_body_dir());
        let is_shoe_rotated = self.swaps_axes_order() ^ body.is_shoe_rotated();

        let joint_dir = body.z_conn_dir();
        let joint_pos = if self.rotate_direction(joint_dir).is_positive() == joint_dir.is_positive()
        {
            body.joint_pos()
        } else {
            body.joint_pos().opposite()
        };
        let result = VoxelBody::new_with(other_body_dir, is_shoe_rotated, joint_pos);
        debug_assert_eq!(self.rotate_direction(joint_dir), result.z_conn_dir());
        result
    }

    pub fn rotate_world(&self, world: &VoxelWorld) -> VoxelWorld {
        let sizes = self.rotate_sizes(world.sizes());
        let bodies = world.all_bodies().map(|(body, pos)| {
            let body = self.rotate_body(body);
            let pos = self.rotate_pos(pos, world.sizes());
            (body, pos)
        });
        VoxelWorld::from_sizes_and_bodies(sizes, bodies).expect("world rotation shouldn't fail")
    }
}

#[test]
fn test_rotate_sizes() {
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::X,
            neg_axis: [false; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([1, 2, 3])
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Y,
            neg_axis: [false; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([3, 1, 2])
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Z,
            neg_axis: [false; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([2, 3, 1])
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::X,
            neg_axis: [true; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([1, 3, 2])
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Y,
            neg_axis: [true; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([2, 1, 3])
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Z,
            neg_axis: [true; 3],
        }
        .rotate_sizes(VoxelPos([1, 2, 3])),
        VoxelPos([3, 2, 1])
    );
}
