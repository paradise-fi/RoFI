use super::{Connections, VoxelWorldWithConnections};
use crate::atoms::Direction;
use crate::connectivity::get_bodies_connected_to;
use crate::counters::Counter;
use crate::module_move::Move;
use crate::module_repr::{get_all_module_reprs, get_other_body, is_module_repr};
use crate::pos::Pos;
use crate::voxel::PosVoxel;
use crate::voxel_world::{NormVoxelWorld, VoxelWorld};
use rustc_hash::FxHashMap;

/// Returns all possible next worlds from moving joint
///
/// VoxelWorldWithConnections is always normalized
fn get_joint_move_successors<TWorld, TConnections>(
    world: &VoxelWorldWithConnections<TWorld, TConnections>,
    module: PosVoxel<TWorld::IndexType>,
) -> impl Iterator<Item = VoxelWorldWithConnections<TWorld, TConnections>> + '_
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: num::Integer + std::hash::Hash,
    TConnections: Connections<IndexType = TWorld::IndexType>,
{
    let other_body = get_other_body(module, world.world()).unwrap();
    world
        .split_by_module(module.0)
        .into_iter()
        .flat_map(move |split| {
            Move::all_possible_moves(module.1, other_body.1).filter_map(move |module_move| {
                Counter::new_move();
                if let Some(result) = module_move.apply(module, &split) {
                    let pos_identity = split
                        .underlying_world()
                        .all_voxels()
                        .map(|(pos, _)| (pos, pos))
                        .collect::<FxHashMap<_, _>>();
                    let pos_mapping = module_move
                        .apply_to_mapping(module, &split, pos_identity)
                        .unwrap();
                    let rot = module_move.get_rotation(module.1, other_body.1);

                    Some(VoxelWorldWithConnections::new_assume_connected(
                        result,
                        world.connections.all_connections().map(|(pos, axis)| {
                            let rotated = split.get_voxel(pos).is_none();
                            if !rotated {
                                return (pos_mapping[&pos], axis);
                            }

                            let connection_dir = Direction::new_with(axis, true);
                            let corrected_pos = if rot.dir_is_positive_inverted(connection_dir) {
                                connection_dir.update_position(pos.as_array()).into()
                            } else {
                                pos
                            };
                            (pos_mapping[&corrected_pos], rot.rotate_axis(axis))
                        }),
                    ))
                } else {
                    Counter::move_collided();
                    None
                }
            })
        })
}

/// Returns all possible next worlds from adding connection
///
/// VoxelWorldWithConnections is always normalized
fn get_connect_successors<TWorld, TConnections>(
    world: &VoxelWorldWithConnections<TWorld, TConnections>,
    body: PosVoxel<TWorld::IndexType>,
) -> impl Iterator<Item = VoxelWorldWithConnections<TWorld, TConnections>> + '_
where
    TWorld: NormVoxelWorld,
    TConnections: Connections<IndexType = TWorld::IndexType> + Clone,
{
    get_bodies_connected_to(body, world.world())
        .into_iter()
        .map(move |(other_pos, _)| {
            Counter::new_move();
            let dir = Direction::from_adjacent_positions(body.0.as_array(), other_pos.as_array())
                .unwrap();
            if dir.is_positive() {
                (body.0, dir.axis())
            } else {
                (other_pos, dir.axis())
            }
        })
        .filter(|&(pos, axis)| !world.connections.is_connected(pos, axis))
        .map(|(pos, axis)| {
            let mut connections = world.connections.clone();
            assert!(!connections.is_connected(pos, axis));
            connections.connect(pos, axis);
            VoxelWorldWithConnections::new_assume_normalized_and_connected(
                world.world.clone(),
                connections,
            )
        })
}

/// Returns all possible next worlds from removing connection
///
/// VoxelWorldWithConnections is always normalized
fn get_disconnect_successors<TWorld, TConnections>(
    world: &VoxelWorldWithConnections<TWorld, TConnections>,
    body_pos: Pos<TWorld::IndexType>,
) -> impl Iterator<Item = VoxelWorldWithConnections<TWorld, TConnections>> + '_
where
    TWorld: NormVoxelWorld,
    TConnections: Connections<IndexType = TWorld::IndexType> + Clone,
{
    world
        .connections
        .connections_from(body_pos)
        .into_iter()
        .filter_map(move |axis| {
            let mut connections = world.connections.clone();
            assert!(connections.is_connected(body_pos, axis));
            connections.disconnect(body_pos, axis);

            let result =
                VoxelWorldWithConnections::new_assume_normalized(world.world.clone(), connections);
            if result.is_some() {
                Counter::new_move();
            }
            result
        })
}

/// Returns all possible next worlds
///
/// VoxelWorldWithConnections is always normalized
pub fn all_next_worlds<TWorld, TConnections>(
    world: &VoxelWorldWithConnections<TWorld, TConnections>,
) -> impl Iterator<Item = VoxelWorldWithConnections<TWorld, TConnections>> + '_
where
    TWorld: NormVoxelWorld,
    TWorld::IndexType: num::Integer + std::hash::Hash,
    TConnections: Connections<IndexType = TWorld::IndexType> + Clone,
{
    Counter::new_successors_call();
    get_all_module_reprs(world.world()).flat_map(move |module| {
        assert!(is_module_repr(module.1));
        Counter::new_module();
        let other_body = get_other_body(module, world.world()).unwrap();

        get_joint_move_successors(world, module)
            .chain(get_connect_successors(world, module))
            .chain(get_connect_successors(world, other_body))
            .chain(get_disconnect_successors(world, module.0))
            .chain(get_disconnect_successors(world, other_body.0))
    })
}
