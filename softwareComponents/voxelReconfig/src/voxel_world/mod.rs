mod centered;
mod rotated;
mod subworld;
mod world_rotation;

pub mod impls;
pub mod traits;

pub use centered::CenteredVoxelWorld;
pub use rotated::{rotate_voxel, RotatedVoxelWorld};
pub use subworld::VoxelSubworld;
pub use traits::{NormVoxelWorld, VoxelWorld};

use crate::module_repr::get_other_body;
use crate::pos::{minimal_pos_hull, Pos, SizeRanges, Sizes};
use crate::voxel::{get_other_body_pos, PosVoxel};
use iter_fixed::IntoIteratorFixed;
use std::assert_matches::debug_assert_matches;
use world_rotation::WorldRotation;

#[derive(Debug, Clone, amplify::Error)]
pub enum InvalidVoxelWorldError<TIndex: std::fmt::Debug + num::Num + Ord> {
    MissingOtherBody(Pos<TIndex>),
    DuplicateVoxels(Pos<TIndex>),
    NotMinimalSize {
        current: Pos<TIndex>,
        minimal: Sizes<TIndex>,
    },
    NotMinimalSizeRange {
        current: SizeRanges<TIndex>,
        minimal: SizeRanges<TIndex>,
    },
    VoxelOutOfBounds {
        pos: Pos<TIndex>,
        size_ranges: SizeRanges<TIndex>,
    },
}
impl<TIndex: std::fmt::Debug + num::Num + Ord> std::fmt::Display
    for InvalidVoxelWorldError<TIndex>
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            InvalidVoxelWorldError::MissingOtherBody(pos) => {
                write!(f, "Missing other body at {pos:?}")
            }
            InvalidVoxelWorldError::DuplicateVoxels(pos) => {
                write!(f, "Duplicate voxels at {pos:?}")
            }
            InvalidVoxelWorldError::NotMinimalSize { current, minimal } => {
                write!(
                    f,
                    "Not minimal size (current: {current:?}, minimal: {minimal:?})"
                )
            }
            InvalidVoxelWorldError::NotMinimalSizeRange { current, minimal } => {
                write!(
                    f,
                    "Not minimal size range (current: {current:?}, minimal: {minimal:?})"
                )
            }
            InvalidVoxelWorldError::VoxelOutOfBounds { pos, size_ranges } => {
                write!(
                    f,
                    "Voxel is out of bounds (voxel pos: {pos:?}, sizes: {size_ranges:?})"
                )
            }
        }
    }
}

pub fn check_voxel_world<TWorld: VoxelWorld>(
    world: &TWorld,
) -> Result<(), InvalidVoxelWorldError<TWorld::IndexType>> {
    let min_hull = minimal_pos_hull(world.all_voxels().map(|(pos, _)| pos));
    if min_hull != world.size_ranges() {
        return Err(InvalidVoxelWorldError::NotMinimalSizeRange {
            current: world.size_ranges(),
            minimal: min_hull,
        });
    }

    for pos_voxel in world.all_voxels() {
        use InvalidVoxelWorldError::MissingOtherBody;

        let other_body =
            get_other_body(pos_voxel, world).map_err(|_| MissingOtherBody(pos_voxel.0))?;
        if get_other_body_pos(other_body) != pos_voxel.0 {
            return Err(MissingOtherBody(pos_voxel.0));
        }
    }
    Ok(())
}

fn sizes_normalized<IndexType>(sizes: Sizes<IndexType>) -> bool
where
    IndexType: num::Num + Ord,
{
    let Pos {
        x: x_sizes,
        y: y_sizes,
        z: z_sizes,
    } = sizes.get();
    x_sizes >= y_sizes && y_sizes >= z_sizes
}

pub fn is_normalized<TWorld>(world: &TWorld) -> bool
where
    TWorld: NormVoxelWorld,
{
    sizes_normalized(world.sizes())
}

// For sizes in normalized worlds it holds that size.x >= size.y >= size.z
//
// The iterator can return multiple equal worlds (in case the world is symmetrical)
pub fn normalized_eq_worlds<TWorld>(world: &TWorld) -> impl Iterator<Item = TWorld> + '_
where
    TWorld: NormVoxelWorld,
{
    debug_assert_matches!(check_voxel_world(world), Ok(()));
    enum_iterator::all::<WorldRotation>().filter_map(move |world_rot| {
        let transformed_sizes = world_rot.rotate_sizes(world.sizes());
        if sizes_normalized(transformed_sizes) {
            let transformed_world = world_rot.rotate_world(world);
            debug_assert_eq!(transformed_world.sizes(), transformed_sizes);
            debug_assert_matches!(check_voxel_world(&transformed_world), Ok(()));
            debug_assert!(is_normalized(&transformed_world));
            Some(transformed_world)
        } else {
            None
        }
    })
}

pub fn as_one_of_norm_eq_world<TWorld>(world: TWorld) -> TWorld
where
    TWorld: NormVoxelWorld,
{
    if is_normalized(&world) {
        world
    } else {
        normalized_eq_worlds(&world)
            .next()
            .expect("There has to be a normalized version of world")
    }
}

pub fn check_pos<TIndex: num::Num + Ord + Copy + std::fmt::Debug>(
    pos: Pos<TIndex>,
    size_ranges: SizeRanges<TIndex>,
) -> Result<(), InvalidVoxelWorldError<TIndex>> {
    if pos
        .as_array()
        .into_iter_fixed()
        .zip(size_ranges.as_ranges_array())
        .into_iter()
        .all(|(pos, size_range)| size_range.contains(&pos))
    {
        Ok(())
    } else {
        Err(InvalidVoxelWorldError::VoxelOutOfBounds { pos, size_ranges })
    }
}

fn debug_fmt_voxels<IVoxels, TIndex: num::Num + std::fmt::Debug>(
    voxels: IVoxels,
    f: &mut impl std::fmt::Write,
    ws_sep: &str,
    outer_indent: &str,
    inner_indent: &str,
    trail_comma: bool,
) -> std::fmt::Result
where
    IVoxels: Iterator<Item = PosVoxel<TIndex>>,
{
    let mut voxels = voxels.peekable();

    if voxels.peek().is_none() {
        return f.write_str("[]");
    }

    f.write_char('[')?;

    let mut comma_sep = None;
    for (pos, voxel) in voxels {
        if let Some(comma_sep) = comma_sep {
            f.write_char(comma_sep)?;
        }
        comma_sep = Some(',');
        f.write_str(ws_sep)?;

        f.write_str(outer_indent)?;
        f.write_str(inner_indent)?;
        f.write_fmt(format_args!("{pos:?}: {voxel:?}"))?;
    }

    if trail_comma {
        assert!(comma_sep.is_some());
        f.write_char(',')?;
    }

    f.write_str(ws_sep)?;
    f.write_str(outer_indent)?;
    f.write_char(']')
}
