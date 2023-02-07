use anyhow::Result;
use clap::Parser;
use rofi_voxel_cli::{FileInput, LogArgs};
use rofi_voxel_reconfig::reconfig;
use rofi_voxel_reconfig::voxel_world::impls::{MapVoxelWorld, MatrixVoxelWorld, SortvecVoxelWorld};
use rofi_voxel_reconfig::voxel_world::NormVoxelWorld;

type IndexType = i8;

#[derive(Debug, Clone, Copy, amplify::Display, clap::ValueEnum)]
#[display(lowercase)]
enum VoxelWorldRepr {
    Map,
    Matrix,
    Sortvec,
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

fn run_voxel_reconfig<TWorld>(
    init: rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>,
    goal: rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>,
) -> Result<Vec<rofi_voxel_reconfig::serde::VoxelWorld<TWorld::IndexType>>>
where
    TWorld: NormVoxelWorld + Eq + std::hash::Hash,
    TWorld::IndexType: 'static + num::Integer + std::hash::Hash + Send + Sync,
{
    let (init, _min_pos) = init.to_world_and_min_pos()?;
    let (goal, _min_pos) = goal.to_world_and_min_pos()?;

    let reconfig_sequence = reconfig::compute_reconfig_path::<TWorld>(&init, goal)?;
    let reconfig_sequence = reconfig_sequence
        .iter()
        .map(|world| rofi_voxel_reconfig::serde::VoxelWorld::from_world(world.as_ref()))
        .collect::<Vec<_>>();
    Ok(reconfig_sequence)
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let InputWorlds { init, goal } = args.get_worlds()?;

    let reconfig_sequence = match args.repr {
        VoxelWorldRepr::Map => run_voxel_reconfig::<MapVoxelWorld<_>>(init, goal)?,
        VoxelWorldRepr::Matrix => run_voxel_reconfig::<MatrixVoxelWorld<_>>(init, goal)?,
        VoxelWorldRepr::Sortvec => run_voxel_reconfig::<SortvecVoxelWorld<_>>(init, goal)?,
    };

    if args.short {
        serde_json::to_writer(std::io::stdout(), &reconfig_sequence)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &reconfig_sequence)?;
    }

    Ok(())
}
