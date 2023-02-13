//! Heuristic based on finding mappings by going through all mappings
//! and evaluating the difference of worlds fixed to these mappings.

use self::graph::{Graph, Mapping, Node};
use crate::voxel::{JointPosition, Voxel};
use crate::voxel_world::NormVoxelWorld;
use std::marker::PhantomData;

pub struct NaiveHeuristic<TWorld>
where
    TWorld: NormVoxelWorld,
{
    goal: Graph<TWorld::IndexType>,
    __phantom: PhantomData<TWorld>,
}

impl<TWorld> NaiveHeuristic<TWorld>
where
    TWorld: NormVoxelWorld,
{
    pub fn get_best_mappings<'a>(
        &'a self,
        other: &'a Graph<TWorld::IndexType>,
    ) -> impl Iterator<Item = Mapping<'a, TWorld::IndexType>> {
        self.goal.get_all_mappings(other)
    }

    fn voxel_joint_diff(goal: Voxel, other: Voxel) -> usize {
        match (goal.joint_pos(), other.joint_pos()) {
            (JointPosition::Zero, JointPosition::Zero) => 0,
            (JointPosition::Zero, _) | (_, JointPosition::Zero) => 1,
            _ => 0,
        }
    }
    pub fn voxel_diff(goal: [Voxel; 2], other: [Voxel; 2]) -> usize {
        let [goal_a, goal_b] = goal;
        let [other_a, other_b] = other;

        let goal_gamma_rot = goal_a.shoe_rotated() != goal_b.shoe_rotated();
        let other_gamma_rot = other_a.shoe_rotated() != other_b.shoe_rotated();
        let gamma_different = goal_gamma_rot != other_gamma_rot;

        std::cmp::min(
            Self::voxel_joint_diff(goal_a, other_a) + Self::voxel_joint_diff(goal_b, other_b),
            Self::voxel_joint_diff(goal_a, other_b) + Self::voxel_joint_diff(goal_b, other_a),
        ) + if gamma_different { 1 } else { 0 }
    }

    pub fn evaluate_mapping(mapping: &Mapping<TWorld::IndexType>) -> usize {
        let goal_graph = mapping.orig_graph();
        let other_graph = mapping.other_graph();

        let mut missing_edges = 0;
        for (from_node, to_node) in goal_graph.edges() {
            if !other_graph.has_edge(mapping.to_other(from_node), mapping.to_other(to_node)) {
                missing_edges += 1;
            }
        }
        assert!(missing_edges % 2 == 0, "Edges are added in both dirs");
        missing_edges /= 2;

        let mut voxel_diffs = 0;
        for (goal_node, other_node) in mapping.nodes() {
            let to_voxels = |node: &Node<_>| node.module().map(|(_, v)| v);
            voxel_diffs += Self::voxel_diff(to_voxels(goal_node), to_voxels(other_node));
        }

        missing_edges + voxel_diffs
    }

    pub fn compute_heuristic(&mut self, other: &TWorld) -> usize {
        let other_graph = Graph::compute_from(other);

        self.get_best_mappings(&other_graph)
            .map(|mapping| Self::evaluate_mapping(&mapping))
            .min()
            .expect("There has to be a best mapping")
    }
}

impl<TWorld> NaiveHeuristic<TWorld>
where
    TWorld: NormVoxelWorld,
{
    pub fn new(goal: &TWorld) -> Self {
        Self {
            goal: Graph::compute_from(goal),
            __phantom: Default::default(),
        }
    }

    pub fn get_fn<'a>(mut self) -> impl 'a + FnMut(&TWorld) -> usize
    where
        TWorld: 'a,
    {
        move |world: &TWorld| self.compute_heuristic(world)
    }
}

pub mod graph {
    use crate::connectivity::get_bodies_connected_to;
    use crate::module_repr::{get_all_module_reprs, get_other_body, is_module_repr};
    use crate::voxel::PosVoxel;
    use crate::voxel_world::VoxelWorld;
    use itertools::Itertools;

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct Node<TIndex: num::Num> {
        module: [PosVoxel<TIndex>; 2],
        idx: usize,
    }
    impl<TIndex: num::Num> Node<TIndex> {
        fn new(module: [PosVoxel<TIndex>; 2], idx: usize) -> Self {
            Self { module, idx }
        }
        pub fn module(&self) -> &[PosVoxel<TIndex>; 2] {
            &self.module
        }
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    struct Edge {
        connected_to: usize,
    }

    pub struct Graph<TIndex: num::Num> {
        nodes: Vec<Node<TIndex>>,
        edges: Vec<Vec<Edge>>,
    }

    impl<TIndex: num::Num> Graph<TIndex> {
        pub fn compute_from<TWorld>(world: &TWorld) -> Self
        where
            TWorld: VoxelWorld<IndexType = TIndex>,
            TWorld::IndexType: Copy,
        {
            let nodes = get_all_module_reprs(world)
                .zip(0..)
                .map(|(module, idx)| {
                    assert!(is_module_repr(module.1));
                    Node::new([module, get_other_body(module, world).unwrap()], idx)
                })
                .collect::<Vec<_>>();

            let edges = nodes
                .iter()
                .map(|node| {
                    node.module()
                        .iter()
                        .flat_map(|&pos_voxel| get_bodies_connected_to(pos_voxel, world))
                        .filter_map(|(neighbour_pos, _)| {
                            nodes.iter().find(|&connected_to| {
                                connected_to
                                    .module()
                                    .iter()
                                    .any(|(pos, _)| &neighbour_pos == pos)
                            })
                        })
                        .map(|connected_to| {
                            debug_assert!(Some(connected_to) == nodes.get(connected_to.idx));
                            Edge {
                                connected_to: connected_to.idx,
                            }
                        })
                        .collect::<Vec<_>>()
                })
                .collect::<Vec<_>>();

            Self { nodes, edges }
        }

        pub fn nodes(&self) -> &Vec<Node<TIndex>> {
            &self.nodes
        }

        pub fn edges(&self) -> impl Iterator<Item = (&'_ Node<TIndex>, &'_ Node<TIndex>)> {
            self.edges
                .iter()
                .zip(&self.nodes)
                .flat_map(move |(edges, node)| {
                    edges
                        .iter()
                        .map(move |edge| (node, &self.nodes[edge.connected_to]))
                })
        }

        pub fn has_edge(&self, from_node: &Node<TIndex>, to_node: &Node<TIndex>) -> bool {
            debug_assert!(Some(from_node) == self.nodes.get(from_node.idx));
            debug_assert!(Some(to_node) == self.nodes.get(to_node.idx));

            self.edges[from_node.idx]
                .iter()
                .any(|edge| edge.connected_to == to_node.idx)
        }

        pub fn get_all_mappings<'a, TIndexOther: num::Num>(
            &'a self,
            other: &'a Graph<TIndexOther>,
        ) -> impl Iterator<Item = Mapping<TIndex, TIndexOther>> {
            (0..self.nodes.len())
                .permutations(self.nodes.len())
                .map(move |mapping| Mapping::new(mapping, self, other))
        }
    }

    pub struct Mapping<'a, TIndexOrig, TIndexOther = TIndexOrig>
    where
        TIndexOrig: num::Num,
        TIndexOther: num::Num,
    {
        mapping: Vec<usize>,
        orig: &'a Graph<TIndexOrig>,
        other: &'a Graph<TIndexOther>,
    }

    impl<'a, TIndexOrig, TIndexOther> Mapping<'a, TIndexOrig, TIndexOther>
    where
        TIndexOrig: num::Num,
        TIndexOther: num::Num,
    {
        pub fn new(
            mapping: Vec<usize>,
            orig: &'a Graph<TIndexOrig>,
            other: &'a Graph<TIndexOther>,
        ) -> Self {
            assert_eq!(orig.nodes.len(), other.nodes.len());
            assert_eq!(mapping.len(), orig.nodes.len());

            debug_assert_eq!(mapping.iter().unique().count(), mapping.len());
            debug_assert!(mapping
                .iter()
                .all(|other_i| (0..other.nodes.len()).contains(other_i)));

            Self {
                mapping,
                orig,
                other,
            }
        }

        pub fn orig_graph(&self) -> &'a Graph<TIndexOrig> {
            self.orig
        }

        pub fn other_graph(&self) -> &'a Graph<TIndexOther> {
            self.other
        }

        pub fn to_other(&self, orig_node: &Node<TIndexOrig>) -> &Node<TIndexOther> {
            assert!(&self.orig.nodes[orig_node.idx] == orig_node);
            let other_idx = self.mapping[orig_node.idx];
            &self.other.nodes[other_idx]
        }

        pub fn nodes(&self) -> impl Iterator<Item = (&'_ Node<TIndexOrig>, &'_ Node<TIndexOther>)> {
            self.orig
                .nodes
                .iter()
                .zip(&self.mapping)
                .map(|(orig, &other_i)| (orig, &self.other.nodes[other_i]))
        }
    }
}
