#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use itertools::Itertools;
use rofi_voxel::connectivity::ConnectivityGraph;
use rofi_voxel::reconfiguration::all_possible_next_worlds;
use rofi_voxel::voxel_world::VoxelWorld;
use rofi_voxel_cli::{FileInput, LogArgs};
use std::assert_matches::assert_matches;
use std::collections::HashMap;
use std::rc::Rc;
use std::time::SystemTime;

/// Compute world counts after 1 to max steps of the reconfiguration algorithm
#[derive(Debug, Parser)]
struct Cli {
    /// World file in voxel-json format ('-' for standard input)
    world_file: String,
    /// Maximum number of steps
    #[arg(short, long)]
    max: Option<usize>,
    #[clap(flatten)]
    log: LogArgs,
}

impl Cli {
    pub fn get_world(&self) -> Result<rofi_voxel::serde::VoxelWorld> {
        let world = FileInput::from_arg(&self.world_file);
        let world = serde_json::from_reader(world.get_reader()?)?;

        Ok(world)
    }
    pub fn max(&self) -> usize {
        self.max.unwrap_or(usize::MAX)
    }
}

fn get_next_step_counts(
    worlds_to_visit: &[Rc<VoxelWorld>],
    parent_worlds: &mut HashMap<Rc<VoxelWorld>, Option<Rc<VoxelWorld>>>,
    init_bodies_count: usize,
) -> (Vec<Rc<VoxelWorld>>, usize) {
    let mut next_step_worlds = Vec::new();
    let mut next_worlds_count = 0;

    for current in worlds_to_visit {
        assert_matches!(current.check_voxel_world(), Ok(()));
        assert!(current.is_normalized());

        let connectivity_graph = ConnectivityGraph::compute_from(current);
        for new_world in all_possible_next_worlds(current, &connectivity_graph, init_bodies_count) {
            next_worlds_count += 1;

            assert_matches!(new_world.check_voxel_world(), Ok(()));
            assert!(new_world.is_normalized());
            if parent_worlds.contains_key(&new_world) {
                continue;
            }

            parent_worlds.extend(
                new_world
                    .normalized_eq_worlds()
                    .map(|norm_world| (Rc::new(norm_world), Some(current.clone()))),
            );

            assert!(new_world.is_normalized());
            // Get the world as Rc from the parent_worlds
            let new_world = parent_worlds
                .get_key_value(&new_world)
                .expect("Has to contain new_world")
                .0
                .clone();

            next_step_worlds.push(new_world);
        }
    }

    (next_step_worlds, next_worlds_count)
}

fn compute_steps(init: VoxelWorld, max_steps: usize) -> Result<()> {
    assert_matches!(init.check_voxel_world(), Ok(()));
    let init = init.as_one_of_norm_eq_world();
    let init_bodies_count = init.all_bodies().count();

    let mut parent_worlds = init
        .normalized_eq_worlds()
        .map(|init| (Rc::new(init), None))
        .collect::<HashMap<_, _>>();

    log::debug!(
        "parent count: {} (init)",
        init.normalized_eq_worlds().unique().count()
    );

    let init = parent_worlds.keys().next().unwrap().clone();

    let mut worlds_to_visit = vec![init];

    let start_time = SystemTime::now();
    log::debug!(
        "'start': {{ 'new worlds': {}, 'new worlds including eq': {0}, 'all worlds in parent map': {}, 'time ms': {} }},",
        worlds_to_visit.len(),
        parent_worlds.len(),
        SystemTime::now().duration_since(start_time)?.as_millis(),
    );

    for i in 1..=max_steps {
        if worlds_to_visit.is_empty() {
            break;
        }

        let (next_step_worlds, next_worlds_count) =
            get_next_step_counts(&worlds_to_visit, &mut parent_worlds, init_bodies_count);

        println!(
            "'round {i}': {{ 'new worlds': {}, 'new worlds including eq': {}, 'all worlds in parent map': {}, 'time ms': {} }},",
            next_step_worlds.len(),
            next_worlds_count,
            parent_worlds.len(),
            SystemTime::now().duration_since(start_time)?.as_millis(),
        );

        worlds_to_visit = next_step_worlds;
    }

    Ok(())
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let world = args.get_world()?;
    let (world, _min_pos) = world.to_world_and_min_pos()?;
    assert_matches!(world.check_voxel_world(), Ok(()));

    compute_steps(world, args.max())
}
