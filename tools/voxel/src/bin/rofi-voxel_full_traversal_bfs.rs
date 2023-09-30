#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use itertools::Itertools;
use rofi_voxel_cli::{FileErrOutput, FileInput, LogArgs};
use rofi_voxel_reconfig::counters::Counter;
use rofi_voxel_reconfig::reconfig::all_next_worlds_norm;
use rofi_voxel_reconfig::voxel_world::impls::MapVoxelWorld;
use rofi_voxel_reconfig::voxel_world::NormVoxelWorld;
use rofi_voxel_reconfig::voxel_world::{as_one_of_norm_eq_world, normalized_eq_worlds};
use rofi_voxel_reconfig::voxel_world::{check_voxel_world, is_normalized};
use rustc_hash::FxHashMap;
use std::assert_matches::{assert_matches, debug_assert_matches};
use std::io::Write;
use std::rc::Rc;
use std::time::SystemTime;

type IndexType = i8;
type ParentMap<TWorld> = FxHashMap<Rc<TWorld>, Option<Rc<TWorld>>>;

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
    /// Log counters to file ('-' for error output)
    #[arg(short, long)]
    log_counters: Option<String>,
    #[clap(flatten)]
    log: LogArgs,
}

impl Cli {
    pub fn get_world(&self) -> Result<rofi_voxel_reconfig::serde::VoxelWorld<IndexType>> {
        let world = FileInput::from_arg(&self.world_file);
        let world = serde_json::from_reader(world.get_reader()?)?;

        Ok(world)
    }
    pub fn max(&self) -> usize {
        self.max.unwrap_or(usize::MAX)
    }
}

#[derive(Debug, Clone, Copy)]
struct RoundData {
    new_worlds: usize,
    new_worlds_including_eq: usize,
    all_worlds_in_parent_map: usize,
    round_time: std::time::Duration,
}

impl RoundData {
    pub fn as_json_str(&self) -> String {
        let Self {
            new_worlds,
            new_worlds_including_eq,
            all_worlds_in_parent_map,
            round_time,
        } = self;
        let round_time_us = round_time.as_micros();
        format!("{{\"new worlds\": {new_worlds}, \"new worlds including eq\": {new_worlds_including_eq}, \"all worlds in parent map\": {all_worlds_in_parent_map}, \"round time [us]\": {round_time_us}}}")
    }
}

#[derive(Debug)]
struct ResultsData {
    voxel_count: usize,
    init_eq_worlds: usize,
    rounds: Vec<RoundData>,
    last_time: SystemTime,
}

impl ResultsData {
    pub fn new(voxel_count: usize, init_eq_worlds: usize) -> Self {
        log::info!("Starting with voxel_count: {voxel_count}, eq_worlds: {init_eq_worlds}");
        Self {
            voxel_count,
            init_eq_worlds,
            rounds: Vec::new(),
            last_time: SystemTime::now(),
        }
    }

    pub fn as_json_str(&self) -> String {
        format!(
            "{{\n  \"voxel count\": {},\n  \"start\": {},\n  \"rounds\": [\n    {}\n  ]\n}}",
            self.voxel_count,
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
        let now = SystemTime::now();
        self.rounds.push(RoundData {
            new_worlds,
            new_worlds_including_eq,
            all_worlds_in_parent_map,
            round_time: now.duration_since(self.last_time).unwrap_or_default(),
        });
        self.last_time = now;
        log::info!(
            "round {}: {}",
            self.rounds.len(),
            self.rounds.last().unwrap().as_json_str()
        );
    }

    pub fn get_start(&self) -> RoundData {
        RoundData {
            new_worlds: 1,
            new_worlds_including_eq: self.init_eq_worlds,
            all_worlds_in_parent_map: self.init_eq_worlds,
            round_time: std::time::Duration::ZERO,
        }
    }
}

fn run_bfs_layer<TWorld>(
    worlds_to_visit: &[Rc<TWorld>],
    parent_map: &mut ParentMap<TWorld>,
) -> (Vec<Rc<TWorld>>, usize)
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    let mut next_step_worlds = Vec::new();
    let mut next_worlds_count = 0;

    for current in worlds_to_visit {
        debug_assert_matches!(check_voxel_world(current.as_ref()), Ok(()));
        debug_assert!(is_normalized(current.as_ref()));
        for new_world in all_next_worlds_norm(current.as_ref()) {
            debug_assert_matches!(check_voxel_world(&new_world), Ok(()));
            debug_assert!(is_normalized(&new_world));

            if parent_map.contains_key(&new_world) {
                debug_assert!(
                    normalized_eq_worlds(&new_world)
                        .all(|norm_world| parent_map.contains_key(&norm_world)),
                    "parent map contains a normalized variant, but not itself"
                );
                continue;
            }

            debug_assert!(
                normalized_eq_worlds(&new_world)
                    .all(|norm_world| !parent_map.contains_key(&norm_world)),
                "parent map contains itself but not some normalized variant"
            );

            next_worlds_count += normalized_eq_worlds(&new_world).count();
            let mut norm_worlds = normalized_eq_worlds(&new_world).map(Rc::new).peekable();
            let new_world = norm_worlds.peek().expect("No normalized variant").clone();
            debug_assert!(is_normalized(new_world.as_ref()));

            Counter::saved_new_unique_state();
            parent_map.extend(norm_worlds.map(|norm_world| (norm_world, Some(current.clone()))));

            next_step_worlds.push(new_world);
        }
    }

    (next_step_worlds, next_worlds_count)
}

fn compute_steps<TWorld>(init: TWorld, max_steps: usize) -> Result<ResultsData>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    assert_matches!(check_voxel_world(&init), Ok(()));
    let init = as_one_of_norm_eq_world(init);

    let mut init_worlds = normalized_eq_worlds(&init).map(Rc::new).peekable();
    let init = init_worlds.peek().expect("No normalized variant").clone();
    let mut parent_map = init_worlds
        .map(|init| (init, None))
        .collect::<ParentMap<_>>();

    let mut results_data = ResultsData::new(init.all_voxels().count(), 1);
    let mut worlds_to_visit = Vec::from([init]);

    for _ in 0..max_steps {
        if worlds_to_visit.is_empty() {
            break;
        }

        let (next_step_worlds, next_worlds_count) =
            run_bfs_layer(&worlds_to_visit, &mut parent_map);

        results_data.add_round(next_step_worlds.len(), next_worlds_count, parent_map.len());

        worlds_to_visit = next_step_worlds;
    }

    Ok(results_data)
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let log_counters = if let Some(log_counters) = &args.log_counters {
        Counter::start()?;

        let log_counters = FileErrOutput::from_arg(log_counters);
        log_counters.touch()?;
        Some(log_counters)
    } else {
        None
    };

    let world = args.get_world()?;
    let (world, _min_pos) = world.to_world_and_min_pos::<MapVoxelWorld<_>>()?;
    assert_matches!(check_voxel_world(&world), Ok(()));

    let results_data = compute_steps(world, args.max())?;

    let results_data = results_data.as_json_str();
    if let Some(output_file) = &args.output {
        std::fs::File::create(output_file)?.write_all(results_data.as_bytes())?;
    } else {
        println!("{results_data}");
    }

    if let Some(log_counters) = log_counters {
        serde_json::to_writer_pretty(log_counters.get_writer()?, &Counter::get_results(true))?;
    }

    Ok(())
}
