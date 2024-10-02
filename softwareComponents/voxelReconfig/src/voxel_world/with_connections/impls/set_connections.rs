use super::super::Connections;
use crate::atoms::Axis;
use crate::pos::ord::OrdPos;
use crate::pos::Pos;
use enum_iterator::Sequence;
use std::collections::BTreeSet;

pub trait SetConnectionsIndex:
    num::Signed + Ord + Copy + std::hash::Hash + std::fmt::Debug
{
}
impl<T> SetConnectionsIndex for T where
    Self: num::Signed + Ord + Copy + std::hash::Hash + std::fmt::Debug
{
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct SetConnections<IndexType: SetConnectionsIndex> {
    connections: BTreeSet<(OrdPos<IndexType>, Axis)>,
}

impl<TIndex: SetConnectionsIndex> Default for SetConnections<TIndex> {
    fn default() -> Self {
        Self {
            connections: Default::default(),
        }
    }
}

fn iter_adaptor<TIndex: num::Num + Copy>(value: &(OrdPos<TIndex>, Axis)) -> (Pos<TIndex>, Axis) {
    let &(OrdPos(pos), axis) = value;
    (pos, axis)
}

impl<TIndex: SetConnectionsIndex> Connections for SetConnections<TIndex> {
    type IndexType = TIndex;
    type ConnectionIter<'a> = impl 'a + Iterator<Item = (Pos<Self::IndexType>, Axis)>
    where
        Self: 'a;

    fn connect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis) {
        self.connections.insert((OrdPos(pos_from), connected_to));
    }

    fn disconnect(&mut self, pos_from: Pos<Self::IndexType>, connected_to: Axis) {
        self.connections.remove(&(OrdPos(pos_from), connected_to));
    }

    fn is_connected(&self, pos_from: Pos<Self::IndexType>, connected_to: Axis) -> bool {
        self.connections.contains(&(OrdPos(pos_from), connected_to))
    }

    fn connections_from(&self, pos_from: Pos<Self::IndexType>) -> smallvec::SmallVec<[Axis; 3]> {
        let first_axis = Axis::first().unwrap();
        let last_axis = Axis::last().unwrap();
        self.connections
            .range((OrdPos(pos_from), first_axis)..=(OrdPos(pos_from), last_axis))
            .map(|&(_, axis)| axis)
            .collect()
    }

    fn all_connections(&self) -> Self::ConnectionIter<'_> {
        self.connections.iter().map(iter_adaptor)
    }

    fn from_connections(
        connections_iter: impl IntoIterator<Item = (Pos<Self::IndexType>, Axis)>,
    ) -> Self {
        Self {
            connections: connections_iter
                .into_iter()
                .map(|(pos, axis)| (OrdPos(pos), axis))
                .collect(),
        }
    }
}
