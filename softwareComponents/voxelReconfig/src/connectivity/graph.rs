use super::get_bodies_connected_to_body;
use crate::atoms::Subset;
use crate::module_repr::{get_module_repr_pos, is_module_repr};
use crate::pos::VoxelPos;
use crate::voxel::body::get_neighbour_pos;
use crate::voxel::VoxelBodyWithPos;
use crate::voxel_world::{VoxelSubworld, VoxelWorld};
use bimap::BiHashMap;
use rs_graph::algorithms::{subgraph, Item};
use rs_graph::linkedlistgraph::Node;
use rs_graph::traits::{GraphSize, Undirected};
use rs_graph::{Buildable, Builder, LinkedListGraph};
use std::collections::HashMap;
use std::sync::atomic::Ordering;

type GraphType = LinkedListGraph;

// Fix bug in `rs_graph::algorithms::is_connected`
pub fn is_connected<'g, TGraph>(graph: &'g TGraph) -> bool
where
    TGraph: rs_graph::IndexGraph<'g>,
{
    graph.num_nodes() <= 1 || rs_graph::algorithms::is_connected(&graph)
}

pub struct ConnectivityGraph<'a> {
    world: &'a VoxelWorld,
    graph: GraphType,
    mapping: BiHashMap<VoxelPos, Node>,
}

impl<'a> ConnectivityGraph<'a> {
    pub fn graph(&self) -> &GraphType {
        &self.graph
    }

    fn add_connectivity_nodes(
        world: &'a VoxelWorld,
        mapping: &mut BiHashMap<VoxelPos, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) {
        for (_, module_pos) in world.all_bodies() {
            let node_ids = graph_builder.add_node();
            mapping.insert_no_overwrite(module_pos, node_ids).unwrap();
        }
    }

    fn add_connectivity_edges(
        world: &'a VoxelWorld,
        mapping: &BiHashMap<VoxelPos, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) {
        for (&pos, &i) in mapping.iter() {
            let body = world.get_body(pos).expect("Invalid world");

            let other_body_pos = get_neighbour_pos((body, pos)).unwrap();
            let &other_body_i = mapping.get_by_left(&other_body_pos).expect("Invalid world");
            graph_builder.add_edge(i, other_body_i);

            for (_, other_pos) in get_bodies_connected_to_body((body, pos), world) {
                let &other_i = mapping.get_by_left(&other_pos).expect("Invalid mapping");
                graph_builder.add_edge(i, other_i);
            }
        }
    }

    pub fn compute_from(world: &'a VoxelWorld) -> Self {
        let mut graph_builder = GraphType::new_builder();
        let mut mapping = BiHashMap::new();

        Self::add_connectivity_nodes(world, &mut mapping, &mut graph_builder);
        Self::add_connectivity_edges(world, &mapping, &mut graph_builder);

        Self {
            world,
            graph: graph_builder.into_graph(),
            mapping,
        }
    }

    fn graph_with_separated_bodies_of(&self, module: VoxelBodyWithPos) -> GraphType {
        assert!(is_module_repr(module.0));

        let pos_a = module.1;
        let pos_b = get_neighbour_pos(module).expect("Invalid world");
        subgraph(&self.graph, |i| match i {
            Item::Node(_) => true,
            Item::Edge(e) => {
                let enodes = self.graph.enodes(e);
                let enodes = [enodes.0, enodes.1]
                    .map(|e| *self.mapping.get_by_right(&e).expect("Node not in mapping"));
                !enodes.contains(&pos_a) || !enodes.contains(&pos_b)
            }
        })
    }

    fn get_module_repr_node_to_id_mapping(
        &self,
        split_module_pos: VoxelPos,
    ) -> (HashMap<Node, usize>, usize) {
        let module_repr_pos_to_id = self
            .mapping
            .iter()
            .map(|(&pos, _)| pos)
            .filter(|&pos| {
                let body = self.world.get_body(pos).expect("Invalid mapping");
                is_module_repr(body) && pos != split_module_pos
            })
            .zip(0..)
            .collect::<HashMap<_, _>>();

        let id_count = module_repr_pos_to_id.len();

        let module_repr_node_to_id_mapping = self
            .mapping
            .iter()
            .filter_map(|(&pos, &node)| {
                let body = self.world.get_body(pos).expect("Invalid mapping");
                let module_repr_pos = get_module_repr_pos((body, pos));
                module_repr_pos_to_id
                    .get(&module_repr_pos)
                    .map(|&id| (node, id))
            })
            .collect::<HashMap<_, _>>();

        (module_repr_node_to_id_mapping, id_count)
    }

    fn get_is_selected_predicate(
        &self,
        split_module: VoxelBodyWithPos,
    ) -> (impl Fn(Node, &Subset) -> bool + Clone + '_, usize) {
        let (module_repr_node_to_id, id_count) =
            self.get_module_repr_node_to_id_mapping(split_module.1);

        let module_node = *self
            .mapping
            .get_by_left(&split_module.1)
            .expect("Invalid mapping");
        let other_module_body_pos = *self
            .mapping
            .get_by_left(&get_neighbour_pos(split_module).expect("Invalid world"))
            .expect("Invalid mapping");

        let is_selected = move |node: Node, selection: &Subset| -> bool {
            if let Some(&node_index) = module_repr_node_to_id.get(&node) {
                selection.has_elem(node_index)
            } else if node == module_node {
                true
            } else {
                assert_eq!(node, other_module_body_pos);
                false
            }
        };
        (is_selected, id_count)
    }

    pub fn all_cuts_by_module(
        &self,
        module: VoxelBodyWithPos,
    ) -> impl Iterator<Item = VoxelSubworld> + '_ {
        assert!(is_module_repr(module.0));

        let (is_selected, id_count) = self.get_is_selected_predicate(module);

        let graph_base = self.graph_with_separated_bodies_of(module);

        debug_assert_eq!(self.graph.num_nodes(), 2 + id_count * 2);
        Subset::iter_all(id_count).filter_map(move |selection| {
            if Self::is_valid_selection(&graph_base, |node| is_selected(node, &selection)) {
                crate::reconfiguration::TRUE_CUTS.fetch_add(1, Ordering::Relaxed);
                let is_selected = is_selected.clone();
                Some(VoxelSubworld::new(self.world, move |pos| {
                    is_selected(
                        *self.mapping.get_by_left(&pos).expect("Invalid mapping"),
                        &selection,
                    )
                }))
            } else {
                crate::reconfiguration::FALSE_CUTS.fetch_add(1, Ordering::Relaxed);
                None
            }
        })
    }

    fn get_subgraph<TSelection>(graph_base: &GraphType, selection: &TSelection) -> GraphType
    where
        TSelection: Fn(<GraphType as rs_graph::traits::GraphType>::Node) -> bool,
    {
        subgraph(graph_base, |i| match i {
            Item::Node(node) => selection(node),
            Item::Edge(_) => true,
        })
    }

    fn is_valid_selection<TSelection>(graph_base: &GraphType, selection: TSelection) -> bool
    where
        TSelection: Fn(<GraphType as rs_graph::traits::GraphType>::Node) -> bool,
    {
        [
            Self::get_subgraph(graph_base, &selection),
            Self::get_subgraph(graph_base, &|node| !selection(node)),
        ]
        .iter()
        .all(is_connected)
    }
}
