pub mod impls;
pub mod metric;
pub mod reconfig;

use super::world_rotation::WorldRotation;
use super::{as_one_of_norm_eq_world, as_one_of_norm_eq_world_with_rot};
use super::{NormVoxelWorld, VoxelSubworld, VoxelWorld};
use crate::atoms::{Axis, Direction};
use crate::connectivity::get_bodies_connected_to;
use crate::pos::ord::OrdPos;
use crate::pos::{Pos, Sizes};
use crate::voxel::get_other_body_pos;
use crate::voxel_world::is_normalized;
use itertools::Itertools;
use smallvec::SmallVec;
use std::collections::BTreeSet;
use std::rc::Rc;

pub trait Connections {
    type IndexType: num::Signed + Copy;
    type ConnectionIter<'a>: Iterator<Item = (Pos<Self::IndexType>, Axis)>
    where
        Self: 'a;

    fn connect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis);
    fn disconnect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis);
    fn is_connected(&self, pos_from: Pos<Self::IndexType>, connected_to: Axis) -> bool;

    fn connections_from(&self, pos_from: Pos<Self::IndexType>) -> SmallVec<[Axis; 3]> {
        enum_iterator::all()
            .filter(|&axis| self.is_connected(pos_from, axis))
            .collect()
    }

    fn all_connections(&self) -> Self::ConnectionIter<'_>;

    fn from_connections(
        connections_iter: impl IntoIterator<Item = (Pos<Self::IndexType>, Axis)>,
    ) -> Self;
}

pub fn connection_possible<TWorld: VoxelWorld>(
    world: &TWorld,
    pos_from: Pos<TWorld::IndexType>,
    connect_to: Axis,
) -> bool {
    let body = (pos_from, world.get_voxel(pos_from).unwrap());
    let other_pos =
        Pos::from(Direction::new_with(connect_to, true).update_position(pos_from.as_array()));

    get_bodies_connected_to(body, world)
        .into_iter()
        .any(|(pos, _)| pos == other_pos)
}

pub fn get_connected_to<TConnections: Connections>(
    connections: &TConnections,
    pos_from: Pos<TConnections::IndexType>,
) -> impl Iterator<Item = Pos<TConnections::IndexType>> + '_ {
    let positive = enum_iterator::all().filter_map(move |axis| {
        if connections.is_connected(pos_from, axis) {
            let other_dir = Direction::new_with(axis, true);
            Some(Pos::from(other_dir.update_position(pos_from.as_array())))
        } else {
            None
        }
    });

    let negative = enum_iterator::all().filter_map(move |axis| {
        let other_dir = Direction::new_with(axis, false);
        let other_pos = Pos::from(other_dir.update_position(pos_from.as_array()));
        if connections.is_connected(other_pos, axis) {
            Some(other_pos)
        } else {
            None
        }
    });

    positive.chain(negative)
}

fn correction_for_rotating_connection<TIndex: num::Signed>(
    rotation: &WorldRotation,
    orig_conn_pos: Pos<TIndex>,
    orig_conn_axis: Axis,
) -> Pos<TIndex> {
    if rotation.is_inverting_axis(orig_conn_axis) {
        Direction::new_with(orig_conn_axis, true)
            .update_position(orig_conn_pos.as_array())
            .into()
    } else {
        orig_conn_pos
    }
}

#[derive(PartialEq, Eq, Hash)]
pub struct VoxelWorldWithConnections<TWorld, TConnections>
where
    TWorld: NormVoxelWorld,
    TConnections: Connections<IndexType = TWorld::IndexType>,
{
    world: Rc<TWorld>,
    connections: TConnections,
}

impl<TWorld, TConnections> VoxelWorldWithConnections<TWorld, TConnections>
where
    TWorld: NormVoxelWorld,
    TConnections: Connections<IndexType = TWorld::IndexType>,
{
    pub fn new_all_connected(world: TWorld) -> Self {
        let world = Rc::new(as_one_of_norm_eq_world(world));

        let connections = TConnections::from_connections(
            world
                .all_voxels()
                .map(|(pos, _)| pos)
                .cartesian_product(enum_iterator::all())
                .filter(|&(pos, axis)| connection_possible(world.as_ref(), pos, axis)),
        );

        assert!(Self::is_connected(&world, &connections));
        Self { world, connections }
    }

    pub fn new_assume_normalized_and_connected(
        world: Rc<TWorld>,
        connections: TConnections,
    ) -> Self {
        debug_assert!(is_normalized(world.as_ref()));
        debug_assert!(Self::is_connected(&world, &connections));
        Self { world, connections }
    }

    pub fn new_assume_normalized(world: Rc<TWorld>, connections: TConnections) -> Option<Self> {
        debug_assert!(is_normalized(world.as_ref()));
        if Self::is_connected(&world, &connections) {
            Some(Self { world, connections })
        } else {
            None
        }
    }

    pub fn new_assume_connected<IConnections>(world: TWorld, connections: IConnections) -> Self
    where
        IConnections: IntoIterator<Item = (Pos<TWorld::IndexType>, Axis)>,
    {
        let orig_world_sizes = world.sizes();
        let (norm_world, rotation) = as_one_of_norm_eq_world_with_rot(world);
        let norm_connections = TConnections::from_connections(Self::rotated_connections(
            rotation,
            connections,
            orig_world_sizes,
        ));

        debug_assert!(Self::is_connected(&norm_world, &norm_connections));
        Self {
            world: Rc::new(norm_world),
            connections: norm_connections,
        }
    }

    pub fn new<IConnections>(world: TWorld, connections: IConnections) -> Option<Self>
    where
        IConnections: IntoIterator<Item = (Pos<TWorld::IndexType>, Axis)>,
    {
        let orig_world_sizes = world.sizes();
        let (norm_world, rotation) = as_one_of_norm_eq_world_with_rot(world);
        let norm_connections = TConnections::from_connections(Self::rotated_connections(
            rotation,
            connections,
            orig_world_sizes,
        ));

        if Self::is_connected(&norm_world, &norm_connections) {
            Some(Self {
                world: Rc::new(norm_world),
                connections: norm_connections,
            })
        } else {
            None
        }
    }

    pub fn world(&self) -> &TWorld {
        self.world.as_ref()
    }

    fn rotated_connections<IConnections>(
        rotation: WorldRotation,
        connections: IConnections,
        orig_world_sizes: Sizes<TWorld::IndexType>,
    ) -> impl Iterator<Item = (Pos<TWorld::IndexType>, Axis)>
    where
        IConnections: IntoIterator<Item = (Pos<TWorld::IndexType>, Axis)>,
    {
        connections.into_iter().map(move |(pos, axis)| {
            let corrected_pos = correction_for_rotating_connection(&rotation, pos, axis);
            (
                rotation.rotate_pos(corrected_pos, orig_world_sizes),
                rotation.rotate_axis(axis),
            )
        })
    }

    pub fn get_rotated_connections(&self, rotation: WorldRotation) -> TConnections {
        TConnections::from_connections(Self::rotated_connections(
            rotation,
            self.connections.all_connections(),
            self.world.sizes(),
        ))
    }

    fn traverse_connections(
        world: &TWorld,
        connections: &TConnections,
        visited: &mut BTreeSet<OrdPos<TWorld::IndexType>>,
        mut to_visit: Vec<Pos<TWorld::IndexType>>,
    ) {
        while let Some(pos) = to_visit.pop() {
            let voxel = world
                .get_voxel(pos)
                .expect("to_visit positions have to contain voxel");
            let other_body_pos = get_other_body_pos((pos, voxel));

            let neighbours =
                std::iter::once(other_body_pos).chain(get_connected_to(connections, pos));

            for neighbour in neighbours {
                if !visited.contains(&OrdPos(neighbour)) {
                    visited.insert(OrdPos(neighbour));
                    to_visit.push(neighbour);
                }
            }
        }
    }

    fn is_connected(world: &TWorld, connections: &TConnections) -> bool {
        let first_pos = if let Some((first_pos, _)) = world.all_voxels().next() {
            first_pos
        } else {
            return true;
        };

        let to_visit = vec![first_pos];
        let mut visited = std::iter::once(OrdPos(first_pos)).collect::<BTreeSet<_>>();

        Self::traverse_connections(world, connections, &mut visited, to_visit);

        world
            .all_voxels()
            .all(|(pos, _)| visited.contains(&OrdPos(pos)))
    }

    pub fn split_by_module(
        &self,
        module_pos: Pos<TWorld::IndexType>,
    ) -> Option<VoxelSubworld<'_, TWorld>> {
        let to_visit = get_connected_to(&self.connections, module_pos).collect::<Vec<_>>();

        let mut visited = std::iter::once(module_pos)
            .chain(to_visit.iter().copied())
            .map(OrdPos)
            .collect::<BTreeSet<_>>();

        Self::traverse_connections(&self.world, &self.connections, &mut visited, to_visit);

        let module = (module_pos, self.world.get_voxel(module_pos).unwrap());
        if visited.contains(&OrdPos(get_other_body_pos(module))) {
            None
        } else {
            Some(VoxelSubworld::new_from_set(&self.world, visited))
        }
    }

    pub fn check_is_valid(&self) -> bool {
        Self::is_connected(&self.world, &self.connections)
    }
}
