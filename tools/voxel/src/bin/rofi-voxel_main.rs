use anyhow::Result;
use clap::Parser;
use rofi_voxel_cli::{FileErrOutput, FileInput, LogArgs};
use rofi_voxel_reconfig::algs;
use rofi_voxel_reconfig::counters::Counter;
use rofi_voxel_reconfig::reconfig::heuristic::Heuristic;
use rofi_voxel_reconfig::reconfig::voxel_worlds_graph::VoxelWorldsGraph;
use rofi_voxel_reconfig::voxel_world::as_one_of_norm_eq_world;
use rofi_voxel_reconfig::voxel_world::impls::{MapVoxelWorld, MatrixVoxelWorld, SortvecVoxelWorld};
use rofi_voxel_reconfig::voxel_world::with_connections::heuristic::use_world_heuristic;
use rofi_voxel_reconfig::voxel_world::with_connections::impls::map_connections::MapConnections;
use rofi_voxel_reconfig::voxel_world::with_connections::impls::set_connections::SetConnections;
use rofi_voxel_reconfig::voxel_world::with_connections::reconfig::VoxelWorldsWithConnectionsGraph;
use rofi_voxel_reconfig::voxel_world::with_connections::{Connections, VoxelWorldWithConnections};
use rofi_voxel_reconfig::voxel_world::NormVoxelWorld;
use std::rc::Rc;

type IndexType = i8;

#[derive(Debug, Clone, Copy, amplify::Display, clap::ValueEnum)]
#[display(lowercase)]
enum VoxelWorldRepr {
    Map,
    Matrix,
    Sortvec,
}

#[derive(Debug, Clone, Copy, amplify::Display, clap::ValueEnum)]
#[display(lowercase)]
enum ConnectionsRepr {
    Map,
    Set,
}

#[derive(Debug, Clone, Copy, amplify::Display, clap::ValueEnum)]
#[display(lowercase)]
enum AlgorithmType {
    Bfs,
    AstarZero,
    AstarNaive,
    AstarZeroOpt,
    AstarNaiveOpt,
}

/// Compute RoFI reconfiguration from init to goal by using voxels
#[derive(Debug, Parser)]
struct Cli {
    /// Init world file in voxel-json format ('-' for standard input)
    init_world_file: String,
    /// Goal world file in voxel-json format ('-' for standard input)
    goal_world_file: String,
    /// Voxel world representation
    #[arg(short, long, default_value_t = VoxelWorldRepr::Map)]
    repr: VoxelWorldRepr,
    /// Algorithm for reconfiguration
    #[arg(short, long, default_value_t = AlgorithmType::Bfs)]
    alg: AlgorithmType,
    /// Use connections with specified representation
    #[arg(short, long)]
    connections: Option<ConnectionsRepr>,
    /// Log counters to file ('-' for error output)
    #[arg(short, long)]
    log_counters: Option<String>,
    /// Return result in a short json format
    #[arg(short, long, default_value_t = false)]
    short: bool,
    #[clap(flatten)]
    log: LogArgs,
}

#[derive(Debug)]
struct InputWorlds {
    init: rofi_voxel_reconfig::serde::VoxelWorld<IndexType>,
    goal: rofi_voxel_reconfig::serde::VoxelWorld<IndexType>,
}

impl Cli {
    pub fn get_worlds(&self) -> Result<InputWorlds> {
        let init = FileInput::from_arg(&self.init_world_file);
        let goal = FileInput::from_arg(&self.goal_world_file);

        FileInput::check_max_one_stdin([&init, &goal])?;

        let init = init.read_all()?;
        let goal = goal.read_all()?;

        let init = serde_json::from_str(&init)?;
        let goal = serde_json::from_str(&goal)?;

        Ok(InputWorlds { init, goal })
    }
}

fn run_reconfig_alg<TWorld>(
    init: &TWorld,
    goal: &TWorld,
    alg_type: AlgorithmType,
) -> Result<Vec<Rc<TWorld>>, algs::Error>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    type StateGraph<T> = VoxelWorldsGraph<T>;
    match alg_type {
        AlgorithmType::Bfs => algs::bfs::compute_path::<StateGraph<_>>(init, goal),
        AlgorithmType::AstarZero => {
            algs::astar::compute_path::<StateGraph<_>, _>(init, goal, Heuristic::Zero.get_fn(goal))
        }
        AlgorithmType::AstarNaive => {
            algs::astar::compute_path::<StateGraph<_>, _>(init, goal, Heuristic::Naive.get_fn(goal))
        }
        AlgorithmType::AstarZeroOpt => algs::astar::opt::compute_path::<StateGraph<_>, _>(
            init,
            goal,
            Heuristic::Zero.get_fn(goal),
        ),
        AlgorithmType::AstarNaiveOpt => algs::astar::opt::compute_path::<StateGraph<_>, _>(
            init,
            goal,
            Heuristic::Naive.get_fn(goal),
        ),
    }
}

fn run_reconfig_alg_with_connections<TWorld, TConnections>(
    init: &VoxelWorldWithConnections<TWorld, TConnections>,
    goal: &VoxelWorldWithConnections<TWorld, TConnections>,
    alg_type: AlgorithmType,
) -> Result<Vec<Rc<VoxelWorldWithConnections<TWorld, TConnections>>>, algs::Error>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: num::Integer + std::hash::Hash,
    TConnections: Connections<IndexType = TWorld::IndexType> + Eq + std::hash::Hash + Clone,
    TWorld: 'static,
    TConnections: 'static,
{
    type StateGraph<T, U> = VoxelWorldsWithConnectionsGraph<T, U>;
    match alg_type {
        AlgorithmType::Bfs => algs::bfs::compute_path::<StateGraph<_, _>>(init, goal),
        AlgorithmType::AstarZero => algs::astar::compute_path::<StateGraph<_, _>, _>(
            init,
            goal,
            use_world_heuristic(Heuristic::Zero, goal),
        ),
        AlgorithmType::AstarNaive => algs::astar::compute_path::<StateGraph<_, _>, _>(
            init,
            goal,
            use_world_heuristic(Heuristic::Naive, goal),
        ),
        AlgorithmType::AstarZeroOpt => algs::astar::opt::compute_path::<StateGraph<_, _>, _>(
            init,
            goal,
            use_world_heuristic(Heuristic::Zero, goal),
        ),
        AlgorithmType::AstarNaiveOpt => algs::astar::opt::compute_path::<StateGraph<_, _>, _>(
            init,
            goal,
            use_world_heuristic(Heuristic::Naive, goal),
        ),
    }
}

fn run_voxel_reconfig<TWorld>(
    init: rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>,
    goal: rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>,
    connections_repr: Option<ConnectionsRepr>,
    alg_type: AlgorithmType,
) -> Result<Vec<rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>>>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: 'static + num::Integer + std::hash::Hash + Send + Sync,
    TWorld: 'static,
{
    let (init, _min_pos) = init.to_world_and_min_pos()?;
    let (goal, _min_pos) = goal.to_world_and_min_pos()?;

    let init = as_one_of_norm_eq_world(init);
    let goal = as_one_of_norm_eq_world(goal);

    match connections_repr {
        None => {
            let reconfig_sequence = run_reconfig_alg::<TWorld>(&init, &goal, alg_type)?;
            Ok(reconfig_sequence
                .iter()
                .map(|world| rofi_voxel_reconfig::serde::VoxelWorld::from_world(world.as_ref()))
                .collect())
        }
        Some(ConnectionsRepr::Map) => {
            let reconfig_sequence =
                run_reconfig_alg_with_connections::<TWorld, MapConnections<TWorld::IndexType>>(
                    &VoxelWorldWithConnections::new_all_connected(init),
                    &VoxelWorldWithConnections::new_all_connected(goal),
                    alg_type,
                )?;
            Ok(reconfig_sequence
                .iter()
                .map(|world| rofi_voxel_reconfig::serde::VoxelWorld::from_world(world.world()))
                .collect())
        }
        Some(ConnectionsRepr::Set) => {
            let reconfig_sequence =
                run_reconfig_alg_with_connections::<TWorld, SetConnections<TWorld::IndexType>>(
                    &VoxelWorldWithConnections::new_all_connected(init),
                    &VoxelWorldWithConnections::new_all_connected(goal),
                    alg_type,
                )?;
            Ok(reconfig_sequence
                .iter()
                .map(|world| rofi_voxel_reconfig::serde::VoxelWorld::from_world(world.world()))
                .collect())
        }
    }
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

    let InputWorlds { init, goal } = args.get_worlds()?;

    let reconfig_sequence = match args.repr {
        VoxelWorldRepr::Map => {
            run_voxel_reconfig::<MapVoxelWorld<_>>(init, goal, args.connections, args.alg)?
        }
        VoxelWorldRepr::Matrix => {
            run_voxel_reconfig::<MatrixVoxelWorld<_>>(init, goal, args.connections, args.alg)?
        }
        VoxelWorldRepr::Sortvec => {
            run_voxel_reconfig::<SortvecVoxelWorld<_>>(init, goal, args.connections, args.alg)?
        }
    };

    if args.short {
        serde_json::to_writer(std::io::stdout(), &reconfig_sequence)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &reconfig_sequence)?;
    }

    if let Some(log_counters) = log_counters {
        serde_json::to_writer_pretty(
            log_counters.get_writer()?,
            &Counter::get_results(matches!(args.alg, AlgorithmType::Bfs)),
        )?;
    }

    Ok(())
}
