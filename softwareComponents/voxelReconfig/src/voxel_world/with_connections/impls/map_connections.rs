use super::super::Connections;
use crate::atoms::Axis;
use crate::pos::ord::OrdPos;
use crate::pos::Pos;
use std::collections::{btree_map, BTreeMap};

pub trait MapConnectionsIndex:
    num::Signed + Ord + Copy + std::hash::Hash + std::fmt::Debug
{
}
impl<T> MapConnectionsIndex for T where
    Self: num::Signed + Ord + Copy + std::hash::Hash + std::fmt::Debug
{
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct MapConnections<IndexType: MapConnectionsIndex> {
    connections: BTreeMap<OrdPos<IndexType>, [bool; 3]>,
}

impl<IndexType: MapConnectionsIndex> Default for MapConnections<IndexType> {
    fn default() -> Self {
        Self {
            connections: Default::default(),
        }
    }
}

fn axes_from_inner_value(axes_connected: [bool; 3]) -> smallvec::SmallVec<[Axis; 3]> {
    axes_connected
        .into_iter()
        .zip(enum_iterator::all())
        .filter(|&(axis_connected, _)| axis_connected)
        .map(|(_, axis)| axis)
        .collect()
}

impl<TIndex: MapConnectionsIndex> Connections for MapConnections<TIndex> {
    type IndexType = TIndex;
    type ConnectionIter<'a> = impl 'a + Iterator<Item = (Pos<Self::IndexType>, Axis)>
    where
        Self: 'a;

    fn connect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis) {
        self.connections.entry(OrdPos(pos_from)).or_default()[connected_to.as_index()] = true;
    }

    fn disconnect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis) {
        if let btree_map::Entry::Occupied(mut entry) = self.connections.entry(OrdPos(pos_from)) {
            entry.get_mut()[connected_to.as_index()] = false;
            if entry.get().iter().all(|&value| !value) {
                // Respect default equality
                entry.remove();
            }
        }
    }

    fn is_connected(&self, pos_from: Pos<Self::IndexType>, connected_to: Axis) -> bool {
        if let Some(connections) = self.connections.get(&OrdPos(pos_from)) {
            connections[connected_to.as_index()]
        } else {
            false
        }
    }

    fn connections_from(&self, pos_from: Pos<Self::IndexType>) -> smallvec::SmallVec<[Axis; 3]> {
        self.connections
            .get(&OrdPos(pos_from))
            .copied()
            .map_or_else(Default::default, axes_from_inner_value)
    }

    fn all_connections(&self) -> Self::ConnectionIter<'_> {
        self.connections.iter().flat_map(|(&OrdPos(pos), &axes)| {
            axes_from_inner_value(axes)
                .into_iter()
                .map(move |axis| (pos, axis))
        })
    }

    fn from_connections(
        connections_iter: impl IntoIterator<Item = (Pos<Self::IndexType>, Axis)>,
    ) -> Self {
        let mut result = Self::default();
        for (pos, axis) in connections_iter {
            result.connect(pos, axis);
        }
        result
    }
}
