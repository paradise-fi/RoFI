use super::get_bodies_connected_to;
use crate::atoms::Subset;
use crate::counters::Counter;
use crate::module_repr::{get_module_repr_pos, is_module_repr};
use crate::pos::Pos;
use crate::voxel::{get_other_body_pos, PosVoxel};
use crate::voxel_world::{VoxelSubworld, VoxelWorld};
use bimap::BiHashMap;
use rs_graph::algorithms::{subgraph, Item};
use rs_graph::linkedlistgraph::Node;
use rs_graph::traits::FiniteGraph;
use rs_graph::{Buildable, Builder, LinkedListGraph};
use std::collections::HashMap;
use std::rc::Rc;

type GraphType = LinkedListGraph;

// Fix bug in `rs_graph::algorithms::is_connected`
pub fn is_connected<TGraph>(graph: &TGraph) -> bool
where
    TGraph: rs_graph::IndexGraph,
{
    graph.num_nodes() <= 1 || rs_graph::algorithms::is_connected(&graph)
}

pub struct ConnectivityGraph<'a, TWorld>
where
    TWorld: VoxelWorld,
    TWorld::IndexType: std::hash::Hash,
{
    world: &'a TWorld,
    graph: GraphType,
    mapping: BiHashMap<Pos<TWorld::IndexType>, Node>,
}

impl<'a, TWorld> ConnectivityGraph<'a, TWorld>
where
    TWorld: VoxelWorld,
    TWorld::IndexType: std::hash::Hash,
{
    pub fn compute_from(world: &'a TWorld) -> Self {
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

    pub fn graph(&self) -> &GraphType {
        &self.graph
    }

    fn add_connectivity_nodes(
        world: &TWorld,
        mapping: &mut BiHashMap<Pos<TWorld::IndexType>, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) {
        for (pos, _) in world.all_voxels() {
            let node_ids = graph_builder.add_node();
            mapping
                .insert_no_overwrite(pos, node_ids)
                .expect("Duplicate voxel position");
        }
    }

    fn add_connectivity_edges(
        world: &TWorld,
        mapping: &BiHashMap<Pos<TWorld::IndexType>, Node>,
        graph_builder: &mut <GraphType as rs_graph::Buildable>::Builder,
    ) {
        for (&pos, &node) in mapping.iter() {
            let voxel = world.get_voxel(pos).expect("Invalid world");

            let other_pos = get_other_body_pos((pos, voxel));
            let &other_node = mapping.get_by_left(&other_pos).expect("Invalid world");
            graph_builder.add_edge(node, other_node);

            for (neighbour_pos, _) in get_bodies_connected_to((pos, voxel), world) {
                let &neighbour_node = mapping
                    .get_by_left(&neighbour_pos)
                    .expect("Invalid mapping");
                graph_builder.add_edge(node, neighbour_node);
            }
        }
    }

    fn graph_with_separated_bodies_of(&self, module: PosVoxel<TWorld::IndexType>) -> GraphType {
        assert!(is_module_repr(module.1));

        let pos_a = module.0;
        let pos_b = get_other_body_pos(module);
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

    fn module_node_to_id_mapping(
        &self,
        split_module_pos: Pos<TWorld::IndexType>,
    ) -> (HashMap<Node, u8>, usize) {
        assert!(is_module_repr(
            self.world.get_voxel(split_module_pos).unwrap()
        ));
        let module_pos_to_id = self
            .mapping
            .iter()
            .map(|(&pos, _)| pos)
            .filter(|&pos| {
                let voxel = self.world.get_voxel(pos).expect("Invalid mapping");
                is_module_repr(voxel) && pos != split_module_pos
            })
            .zip(0..)
            .collect::<HashMap<_, _>>();

        let module_node_to_id = self
            .mapping
            .iter()
            .filter_map(|(&pos, &node)| {
                let voxel = self.world.get_voxel(pos).expect("Invalid mapping");
                let module_repr_pos = get_module_repr_pos((pos, voxel));
                // Filters only the `split_module` two voxels
                module_pos_to_id.get(&module_repr_pos).map(|&id| (node, id))
            })
            .collect::<HashMap<_, _>>();

        let id_count = module_pos_to_id.len();
        assert_eq!(module_node_to_id.len(), 2 * id_count);
        (module_node_to_id, id_count)
    }

    fn get_is_selected_predicate(
        &self,
        split_module: PosVoxel<TWorld::IndexType>,
    ) -> (impl Fn(Node, &Subset) -> bool, usize) {
        assert!(is_module_repr(split_module.1));

        let (module_node_to_id, id_count) = self.module_node_to_id_mapping(split_module.0);
        let a_node = *self
            .mapping
            .get_by_left(&split_module.0)
            .expect("Invalid mapping");
        let b_node = *self
            .mapping
            .get_by_left(&get_other_body_pos(split_module))
            .expect("Invalid mapping");

        let is_selected = move |node: Node, selection: &Subset| -> bool {
            if let Some(&node_index) = module_node_to_id.get(&node) {
                selection.has_elem(node_index)
            } else if node == a_node {
                true
            } else {
                assert_eq!(node, b_node);
                false
            }
        };
        (is_selected, id_count)
    }

    pub fn all_cuts_by_module(
        self: Rc<Self>,
        module: PosVoxel<TWorld::IndexType>,
    ) -> impl Iterator<Item = VoxelSubworld<'a, TWorld>> {
        assert!(is_module_repr(module.1));

        let graph_base = self.graph_with_separated_bodies_of(module);
        let (is_selected, id_count) = self.get_is_selected_predicate(module);

        debug_assert_eq!(self.graph.num_nodes(), 2 + id_count * 2);
        Subset::iter_all(id_count).filter_map(move |selection| {
            if is_valid_selection(&graph_base, |node| is_selected(node, &selection)) {
                Counter::new_successful_cut();
                Some(VoxelSubworld::new(self.world, |pos| {
                    let node = *self.mapping.get_by_left(&pos).expect("Invalid mapping");
                    is_selected(node, &selection)
                }))
            } else {
                Counter::new_failed_cut();
                None
            }
        })
    }
}

fn get_subgraph<'a, TSelection>(graph_base: &GraphType, selection: &TSelection) -> GraphType
where
    TSelection: Fn(<GraphType as rs_graph::traits::GraphType>::Node<'a>) -> bool,
{
    subgraph(graph_base, |i| match i {
        Item::Node(node) => selection(node),
        Item::Edge(_) => true,
    })
}

fn is_valid_selection<'a, TSelection>(graph_base: &GraphType, selection: TSelection) -> bool
where
    TSelection: Fn(<GraphType as rs_graph::traits::GraphType>::Node<'a>) -> bool,
{
    [
        get_subgraph(graph_base, &selection),
        get_subgraph(graph_base, &|node| !selection(node)),
    ]
    .iter()
    .all(is_connected)
}
