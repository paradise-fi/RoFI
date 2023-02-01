#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use itertools::Itertools;
use rofi_voxel_cli::{FileInput, LogArgs};
use rofi_voxel_reconfig::connectivity::ConnectivityGraph;
use rofi_voxel_reconfig::reconfiguration::all_possible_next_worlds;
use rofi_voxel_reconfig::voxel_world::VoxelWorld;
use std::assert_matches::assert_matches;
use std::collections::HashMap;
use std::io::Write;
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
    /// File to store the result data (default is standard output)
    #[arg(short, long)]
    output: Option<String>,
    #[clap(flatten)]
    log: LogArgs,
}

impl Cli {
    pub fn get_world(&self) -> Result<rofi_voxel_reconfig::serde::VoxelWorld> {
        let world = FileInput::from_arg(&self.world_file);
        let world = serde_json::from_reader(world.get_reader()?)?;

        Ok(world)
    }
    pub fn max(&self) -> usize {
        self.max.unwrap_or(usize::MAX)
    }
}

#[derive(Debug, Clone, Copy)]
struct ResultData {
    new_worlds: usize,
    new_worlds_including_eq: usize,
    all_worlds_in_parent_map: usize,
    time_since_start: std::time::Duration,
}

impl ResultData {
    pub fn as_json_str(&self) -> String {
        let Self {
            new_worlds,
            new_worlds_including_eq,
            all_worlds_in_parent_map,
            time_since_start,
        } = self;
        let time_since_start_ms = time_since_start.as_micros();
        format!("{{\"new worlds\": {new_worlds}, \"new worlds including eq\": {new_worlds_including_eq}, \"all worlds in parent map\": {all_worlds_in_parent_map}, \"time microsec\": {time_since_start_ms}}}")
    }
}

#[derive(Debug)]
struct ResultsData {
    bodies_count: usize,
    init_eq_worlds: usize,
    rounds: Vec<ResultData>,
    start_time: SystemTime,
}

impl ResultsData {
    pub fn new(bodies_count: usize, init_eq_worlds: usize) -> Self {
        log::info!("Starting with bodies_count: {bodies_count}, eq_worlds: {init_eq_worlds}");
        Self {
            bodies_count,
            init_eq_worlds,
            rounds: Vec::new(),
            start_time: SystemTime::now(),
        }
    }

    pub fn as_json_str(&self) -> String {
        format!(
            "{{\n  \"bodies count\": {},\n  \"start\": {},\n  \"rounds\": [\n    {}\n  ]\n}}",
            self.bodies_count,
            self.get_start().as_json_str(),
            self.rounds
                .iter()
                .map(|round| round.as_json_str())
                .join(",\n    ")
        )
    }

    pub fn add_round(
        &mut self,
        new_worlds: usize,
        new_worlds_including_eq: usize,
        all_worlds_in_parent_map: usize,
    ) {
        self.rounds.push(ResultData {
            new_worlds,
            new_worlds_including_eq,
            all_worlds_in_parent_map,
            time_since_start: SystemTime::now()
                .duration_since(self.start_time)
                .unwrap_or_default(),
        });
        log::info!(
            "round {}: {}",
            self.rounds.len(),
            self.rounds.last().unwrap().as_json_str()
        );
    }

    pub fn get_start(&self) -> ResultData {
        ResultData {
            new_worlds: 1,
            new_worlds_including_eq: self.init_eq_worlds,
            all_worlds_in_parent_map: self.init_eq_worlds,
            time_since_start: std::time::Duration::ZERO,
        }
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

fn compute_steps(init: VoxelWorld, max_steps: usize) -> Result<ResultsData> {
    assert_matches!(init.check_voxel_world(), Ok(()));
    let init = init.as_one_of_norm_eq_world();
    let init_bodies_count = init.all_bodies().count();

    let mut parent_worlds = init
        .normalized_eq_worlds()
        .map(|init| (Rc::new(init), None))
        .collect::<HashMap<_, _>>();

    let mut results_data = ResultsData::new(init_bodies_count, parent_worlds.len());

    let init = parent_worlds.keys().next().unwrap().clone();
    let mut worlds_to_visit = vec![init];
    for _ in 0..max_steps {
        if worlds_to_visit.is_empty() {
            break;
        }

        let (next_step_worlds, next_worlds_count) =
            get_next_step_counts(&worlds_to_visit, &mut parent_worlds, init_bodies_count);

        results_data.add_round(
            next_step_worlds.len(),
            next_worlds_count,
            parent_worlds.len(),
        );

        worlds_to_visit = next_step_worlds;
    }

    Ok(results_data)
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let world = args.get_world()?;
    let (world, _min_pos) = world.to_world_and_min_pos()?;
    assert_matches!(world.check_voxel_world(), Ok(()));

    let results_data = compute_steps(world, args.max())?;

    let results_data = results_data.as_json_str();
    if let Some(output_file) = &args.output {
        std::fs::File::create(output_file)?.write_all(results_data.as_bytes())?;
    } else {
        println!("{results_data}");
    }

    Ok(())
}
