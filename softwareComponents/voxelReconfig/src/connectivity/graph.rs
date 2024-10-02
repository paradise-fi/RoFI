use super::get_bodies_connected_to;
use crate::atoms::{IterEnum, Subset};
use crate::counters::Counter;
use crate::module_repr::{get_module_repr_pos, is_module_repr};
use crate::pos::ord::OrdPos;
use crate::pos::Pos;
use crate::voxel::{get_other_body_pos, PosVoxel};
use crate::voxel_world::{VoxelSubworld, VoxelWorld};
use bimap::BiHashMap;
use rs_graph::algorithms::{subgraph, Item};
use rs_graph::traits::FiniteGraph;
use rs_graph::{Buildable, Builder, LinkedListGraph};
use smallvec::SmallVec;
use std::collections::{BTreeSet, HashMap};
use std::rc::Rc;

type GraphType = LinkedListGraph;
type Node = rs_graph::linkedlistgraph::Node;

fn get_neighbours<TWorld>(
    pos_voxel: PosVoxel<TWorld::IndexType>,
    world: &TWorld,
) -> impl Iterator<Item = Pos<TWorld::IndexType>>
where
    TWorld: VoxelWorld,
{
    std::iter::once(get_other_body_pos(pos_voxel)).chain(
        get_bodies_connected_to(pos_voxel, world)
            .into_iter()
            .map(|(pos, _)| pos),
    )
}

fn get_all_reachable_from_shoe<TWorld>(
    begin_shoe: PosVoxel<TWorld::IndexType>,
    world: &TWorld,
) -> BTreeSet<OrdPos<TWorld::IndexType>>
where
    TWorld: VoxelWorld,
{
    let mut to_visit = get_bodies_connected_to(begin_shoe, world);

    let mut reachable = std::iter::once(begin_shoe.0)
        .chain(to_visit.iter().map(|&(pos, _)| pos))
        .map(OrdPos)
        .collect::<BTreeSet<_>>();

    while let Some(pos_voxel) = to_visit.pop() {
        for neighbour_pos in get_neighbours(pos_voxel, world) {
            if !reachable.contains(&OrdPos(neighbour_pos)) {
                reachable.insert(OrdPos(neighbour_pos));
                to_visit.push((neighbour_pos, world.get_voxel(neighbour_pos).unwrap()));
            }
        }
    }

    reachable
}

type LeafToDependentMap<IndexType> = HashMap<Pos<IndexType>, Pos<IndexType>>;

/// Every leaf will depend on a pos that is in core
/// If there is no core, returns None
fn get_leaves_with_core_dependents<TWorld>(
    world: &TWorld,
) -> Option<LeafToDependentMap<TWorld::IndexType>>
where
    TWorld: VoxelWorld,
    TWorld::IndexType: std::hash::Hash,
{
    let mut leaves_with_dependents = HashMap::new();
    loop {
        let leaves_len = leaves_with_dependents.len();
        for pos_voxel in world.all_voxels() {
            if leaves_with_dependents.contains_key(&pos_voxel.0) {
                continue;
            }

            let core_neighbours = get_neighbours(pos_voxel, world)
                .filter(|other| !leaves_with_dependents.contains_key(other))
                .collect::<SmallVec<[_; 4]>>();
            if core_neighbours.len() > 1 {
                continue;
            }
            if let Some(&depends_on) = core_neighbours.first() {
                leaves_with_dependents.insert(pos_voxel.0, depends_on);
            } else {
                assert_eq!(leaves_with_dependents.len() + 1, world.all_voxels().count());
                return None;
            }
        }
        if leaves_with_dependents.len() == leaves_len {
            break;
        }
    }

    let leaves_with_core_dependents = leaves_with_dependents
        .iter()
        .map(|(&leaf, &depends_on)| {
            let mut depends_on = depends_on;
            while let Some(&next_dependent) = leaves_with_dependents.get(&depends_on) {
                depends_on = next_dependent;
            }
            (leaf, depends_on)
        })
        .collect();
    Some(leaves_with_core_dependents)
}

pub struct ConnectivityGraph<'a, TWorld>
where
    TWorld: VoxelWorld,
    TWorld::IndexType: std::hash::Hash,
{
    world: &'a TWorld,
    core_graph: GraphType,
    core_mapping: BiHashMap<Pos<TWorld::IndexType>, Node>,
    leaves_with_core_dep: Option<LeafToDependentMap<TWorld::IndexType>>,
}

impl<'a, TWorld> ConnectivityGraph<'a, TWorld>
where
    TWorld: VoxelWorld,
    TWorld::IndexType: std::hash::Hash,
{
    pub fn compute_from(world: &'a TWorld) -> Self {
        let leaves_with_core_dep = get_leaves_with_core_dependents(world);

        let mut graph_builder = GraphType::new_builder();
        let mut core_mapping = BiHashMap::new();

        if let Some(leaves_with_core_dep) = &leaves_with_core_dep {
            let core_positions = world
                .all_voxels()
                .map(|(pos, _)| pos)
                .filter(|pos| !leaves_with_core_dep.contains_key(pos));
            Self::add_connectivity_nodes(core_positions, &mut core_mapping, &mut graph_builder);
            Self::add_core_connectivity_edges(world, &core_mapping, &mut graph_builder);
        }

        Self {
            world,
            leaves_with_core_dep,
            core_graph: graph_builder.into_graph(),
            core_mapping,
        }
    }

    pub fn core_graph(&self) -> &GraphType {
        &self.core_graph
    }

    fn add_connectivity_nodes<PosIter>(
        positions: PosIter,
        mapping: &mut BiHashMap<Pos<TWorld::IndexType>, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) where
        PosIter: IntoIterator<Item = Pos<TWorld::IndexType>>,
    {
        for pos in positions {
            let node_ids = graph_builder.add_node();
            mapping
                .insert_no_overwrite(pos, node_ids)
                .expect("Duplicate voxel position");
        }
    }

    fn add_core_connectivity_edges(
        world: &TWorld,
        core_mapping: &BiHashMap<Pos<TWorld::IndexType>, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) {
        for (&pos, &node) in core_mapping.iter() {
            let voxel = world.get_voxel(pos).expect("Invalid world");
            for neighbour_pos in get_neighbours((pos, voxel), world) {
                if let Some(&neighbour_node) = core_mapping.get_by_left(&neighbour_pos) {
                    graph_builder.add_edge(node, neighbour_node);
                }
            }
        }
    }

    fn is_core(&self, pos: Pos<TWorld::IndexType>) -> bool {
        let is_core = self.core_mapping.get_by_left(&pos).is_some();
        let is_leaf = self
            .leaves_with_core_dep
            .as_ref()
            .map_or(true, |leaves| leaves.contains_key(&pos));
        assert_ne!(is_core, is_leaf);
        is_core
    }

    fn core_module_repr_pos_to_idx(
        &self,
        exclude_module_pos: Pos<TWorld::IndexType>,
    ) -> HashMap<Pos<TWorld::IndexType>, u8> {
        assert!(is_module_repr(
            self.world.get_voxel(exclude_module_pos).unwrap()
        ));

        let mut core_module_repr_pos = self
            .core_mapping
            .iter()
            .map(|(&pos, _)| {
                get_module_repr_pos((pos, self.world.get_voxel(pos).expect("Invalid mapping")))
            })
            .filter(|&pos| pos != exclude_module_pos)
            .map(|pos| (pos, 0))
            .collect::<HashMap<_, _>>();

        for ((_, idx), new_idx) in core_module_repr_pos.iter_mut().zip(0..) {
            *idx = new_idx
        }
        core_module_repr_pos
    }

    fn core_node_to_module_idx_mapping(
        &self,
        exclude_module_pos: Pos<TWorld::IndexType>,
    ) -> (HashMap<Node, u8>, usize) {
        assert!(is_module_repr(
            self.world.get_voxel(exclude_module_pos).unwrap()
        ));
        let core_module_repr_pos_to_idx = self.core_module_repr_pos_to_idx(exclude_module_pos);

        let core_node_to_module_repr_idx = self
            .core_mapping
            .iter()
            .filter_map(|(&pos, &node)| {
                let voxel = self.world.get_voxel(pos).expect("Invalid mapping");
                let module_repr_pos = get_module_repr_pos((pos, voxel));
                // Filters only the `exclude_module_pos` two voxels
                core_module_repr_pos_to_idx
                    .get(&module_repr_pos)
                    .map(|&id| (node, id))
            })
            .collect::<HashMap<_, _>>();

        let idx_count = core_module_repr_pos_to_idx.len();
        (core_node_to_module_repr_idx, idx_count)
    }

    pub fn all_cuts_by_module(
        self: Rc<Self>,
        module: PosVoxel<TWorld::IndexType>,
    ) -> impl Iterator<Item = VoxelSubworld<'a, TWorld>> {
        assert!(is_module_repr(module.1));

        let a_pos = module.0;
        let b_pos = get_other_body_pos(module);
        if self.is_core(a_pos) && self.is_core(b_pos) {
            assert!(self.leaves_with_core_dep.is_some());

            let a_node = *self.core_mapping.get_by_left(&a_pos).unwrap();
            let b_node = *self.core_mapping.get_by_left(&b_pos).unwrap();

            let (node_to_idx, idx_count) = self.core_node_to_module_idx_mapping(module.0);

            IterEnum::A(Subset::iter_all(idx_count).filter_map(move |selection| {
                let a_subset_predicate = |node: Node| {
                    if let Some(&idx) = node_to_idx.get(&node) {
                        selection.has_elem(idx)
                    } else {
                        assert!([a_node, b_node].contains(&node));
                        node == a_node
                    }
                };
                let edge_predicate = |edge| {
                    ![(a_node, b_node), (b_node, a_node)].contains(&self.core_graph.enodes(edge))
                };
                if is_split_connected(&self.core_graph, &a_subset_predicate, edge_predicate) {
                    Counter::new_successful_cut();
                    Some(VoxelSubworld::new(self.world, |pos| {
                        if let Some(&core_node) = self.core_mapping.get_by_left(&pos) {
                            a_subset_predicate(core_node)
                        } else {
                            let core_dep = self.leaves_with_core_dep.as_ref().unwrap()[&pos];
                            let core_node = *self
                                .core_mapping
                                .get_by_left(&core_dep)
                                .expect("Dependent is not in core");
                            a_subset_predicate(core_node)
                        }
                    }))
                } else {
                    Counter::new_failed_cut();
                    None
                }
            }))
        } else {
            let a_subset = get_all_reachable_from_shoe(module, self.world);
            assert!(
                !a_subset.contains(&OrdPos(get_other_body_pos(module))),
                "Module shoe is leaf, but the graph is cyclic"
            );
            Counter::new_successful_cut();
            IterEnum::B(std::iter::once(VoxelSubworld::new_from_set(
                self.world, a_subset,
            )))
        }
    }
}

// Fix bug in `rs_graph::algorithms::is_connected`
pub fn is_connected<TGraph>(graph: &TGraph) -> bool
where
    TGraph: rs_graph::IndexGraph,
{
    graph.num_nodes() <= 1 || rs_graph::algorithms::is_connected(&graph)
}

fn is_split_connected<'a, TNodeSelect, TEdgePred>(
    core_graph: &GraphType,
    node_select: &TNodeSelect,
    edge_predicate: TEdgePred,
) -> bool
where
    TNodeSelect: Fn(<GraphType as rs_graph::traits::GraphType>::Node<'a>) -> bool,
    TEdgePred: Fn(<GraphType as rs_graph::traits::GraphType>::Edge<'a>) -> bool,
{
    let subgraphs: [GraphType; 2] = [
        subgraph(core_graph, |i| match i {
            Item::Node(node) => node_select(node),
            Item::Edge(edge) => edge_predicate(edge),
        }),
        subgraph(core_graph, |i| match i {
            Item::Node(node) => !node_select(node),
            Item::Edge(edge) => edge_predicate(edge),
        }),
    ];
    subgraphs.iter().all(is_connected)
}
