use super::NormVoxelWorld;
use crate::atoms;
use crate::pos::{Pos, Sizes};
use crate::voxel::Voxel;
use enum_iterator::Sequence;
use iter_fixed::IntoIteratorFixed;

/// This structure represents rotation of voxel worlds
///
/// The worlds that are being rotated have unsigned indices (going from 0 to size),
/// but the rotation allows inverting some of the axis.
/// So the minimal index will always be at `[0,0,0]` although this doesn't have to correspond
/// with the original position `[0,0,0]`.
///
/// The rotation is given by the axis `x_rotates_to` which corresponds to the original X axis.
/// And by `neg_axis` flags which indicate if the original axis `[X,Y,Z]` is inverted
/// (inverting meaning that original position 0 corresponds to position `size-1` and vice versa).
#[derive(Debug, PartialEq, Eq, Sequence)]
pub struct WorldRotation {
    /// Axis at which rotated [1,0,0] has value 1
    x_rotates_to: atoms::Axis,
    /// Is axis negated before rotation
    neg_axis: [bool; 3],
}

impl Default for WorldRotation {
    fn default() -> Self {
        Self::identity()
    }
}

impl WorldRotation {
    pub fn identity() -> Self {
        Self {
            x_rotates_to: atoms::Axis::X,
            neg_axis: [false; 3],
        }
    }
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

    pub fn rotate_axis(&self, axis: atoms::Axis) -> atoms::Axis {
        debug_assert!(axis.as_index() < self.axes_rotate_to().len());
        self.axes_rotate_to()[axis.as_index()]
    }

    pub fn rotate_direction(&self, direction: atoms::Direction) -> atoms::Direction {
        debug_assert!(direction.axis().as_index() < self.neg_axis.len());
        let is_positive = direction.is_positive() ^ self.neg_axis[direction.axis().as_index()];
        atoms::Direction::new_with(self.rotate_axis(direction.axis()), is_positive)
    }

    fn rotate_as_sizes<TIndex>(&self, sizes: Pos<TIndex>) -> Pos<TIndex>
    where
        TIndex: num::Num + Copy,
    {
        let sizes = sizes.as_array();
        self.rotated_pos_indices()
            .map(|pos_idx| sizes[pos_idx])
            .into()
    }

    pub fn rotate_sizes<TIndex>(&self, sizes: Sizes<TIndex>) -> Sizes<TIndex>
    where
        TIndex: num::Num + Ord + Copy,
    {
        Sizes::new(self.rotate_as_sizes(sizes.get()))
    }

    pub fn rotate_pos<TIndex>(&self, pos: Pos<TIndex>, orig_sizes: Sizes<TIndex>) -> Pos<TIndex>
    where
        TIndex: num::Num + Ord + Copy,
    {
        let negated_pos = pos
            .as_array()
            .into_iter_fixed()
            .zip(orig_sizes.get().as_array())
            .zip(self.neg_axis)
            .map(|((pos, size), is_neg)| {
                if is_neg {
                    size - TIndex::one() - pos
                } else {
                    pos
                }
            });
        self.rotate_as_sizes(negated_pos.collect())
    }

    fn rotate_voxel(&self, voxel: Voxel) -> Voxel {
        let body_dir = self.rotate_direction(voxel.body_dir());
        let shoe_rotated = self.swaps_axes_order() ^ voxel.shoe_rotated();

        let joint_dir = voxel.z_conn_dir();
        let joint_pos = if self.rotate_direction(joint_dir).is_positive() == joint_dir.is_positive()
        {
            voxel.joint_pos()
        } else {
            voxel.joint_pos().opposite()
        };
        let result = Voxel::new_with(body_dir, shoe_rotated, joint_pos);
        debug_assert_eq!(self.rotate_direction(joint_dir), result.z_conn_dir());
        result
    }

    pub fn rotate_world<TWorld: NormVoxelWorld>(&self, world: &TWorld) -> TWorld {
        let sizes = self.rotate_sizes(world.sizes());
        let voxels = world.all_voxels().map(|(pos, voxel)| {
            let voxel = self.rotate_voxel(voxel);
            let pos = self.rotate_pos(pos, world.sizes());
            (pos, voxel)
        });
        TWorld::from_sizes_and_voxels(sizes, voxels).expect("world rotation shouldn't fail")
    }
}

#[test]
fn test_rotate_sizes() {
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::X,
            neg_axis: [false; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([1, 2, 3].into())
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Y,
            neg_axis: [false; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([3, 1, 2].into())
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Z,
            neg_axis: [false; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([2, 3, 1].into())
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::X,
            neg_axis: [true; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([1, 3, 2].into())
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Y,
            neg_axis: [true; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([2, 1, 3].into())
    );
    assert_eq!(
        WorldRotation {
            x_rotates_to: atoms::Axis::Z,
            neg_axis: [true; 3],
        }
        .rotate_sizes(Sizes::new([1, 2, 3].into())),
        Sizes::new([3, 2, 1].into())
    );
}
