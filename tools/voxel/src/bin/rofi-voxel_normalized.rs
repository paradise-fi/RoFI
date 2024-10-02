#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use itertools::Itertools;
use rofi_voxel_cli::{FileInput, LogArgs};
use rofi_voxel_reconfig::voxel_world::impls::MapVoxelWorld;
use rofi_voxel_reconfig::voxel_world::{check_voxel_world, normalized_eq_worlds};
use std::assert_matches::assert_matches;

type IndexType = i8;

/// Compute (unique) normalized versions of voxel world
#[derive(Debug, Parser)]
struct Cli {
    /// World file in voxel-json format ('-' for standard input)
    world_file: String,
    /// Return result in a short json format
    #[arg(short, long, default_value_t = false)]
    short: bool,
    #[clap(flatten)]
    log: LogArgs,
}

#[derive(Debug)]
struct InputWorlds {
    world: rofi_voxel_reconfig::serde::VoxelWorld<IndexType>,
}

impl Cli {
    pub fn get_worlds(&self) -> Result<InputWorlds> {
        let world = FileInput::from_arg(&self.world_file);
        let world = world.read_all()?;
        let world = serde_json::from_str(&world)?;

        Ok(InputWorlds { world })
    }
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let InputWorlds { world } = args.get_worlds()?;
    let (world, _min_pos) = world.to_world_and_min_pos::<MapVoxelWorld<_>>()?;
    assert_matches!(check_voxel_world(&world), Ok(()));

    let norm_worlds = normalized_eq_worlds(&world)
        .unique()
        .map(|eq_world| rofi_voxel_reconfig::serde::VoxelWorld::from_world(&eq_world))
        .collect::<Vec<_>>();

    if args.short {
        serde_json::to_writer(std::io::stdout(), &norm_worlds)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &norm_worlds)?;
    }

    Ok(())
}
