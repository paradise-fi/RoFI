use anyhow::Result;
use clap::Parser;
use rofi_voxel::reconfiguration;
use rofi_voxel_cli::{FileInput, LogArgs};

/// Compute RoFI reconfiguration from init to goal by using voxels
#[derive(Debug, Parser)]
struct Cli {
    /// Init world file in voxel-json format ('-' for standard input)
    init_world_file: String,
    /// Goal world file in voxel-json format ('-' for standard input)
    goal_world_file: String,
    /// Return result in a short json format
    #[arg(short, long, default_value_t = false)]
    short: bool,
    #[clap(flatten)]
    log: LogArgs,
}

#[derive(Debug)]
struct InputWorlds {
    init: rofi_voxel::serde::VoxelWorld,
    goal: rofi_voxel::serde::VoxelWorld,
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

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let InputWorlds { init, goal } = args.get_worlds()?;
    let (init, _min_pos) = init.to_world_and_min_pos()?;
    let (goal, _min_pos) = goal.to_world_and_min_pos()?;

    let reconfig_sequence = reconfiguration::compute_reconfiguration_moves(&init, goal)?;
    let reconfig_sequence = reconfig_sequence
        .iter()
        .map(|world| rofi_voxel::serde::VoxelWorld::from_world(world))
        .collect::<Vec<_>>();

    if args.short {
        serde_json::to_writer(std::io::stdout(), &reconfig_sequence)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &reconfig_sequence)?;
    }

    Ok(())
}
