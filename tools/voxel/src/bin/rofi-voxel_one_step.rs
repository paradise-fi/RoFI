#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use rofi_voxel_cli::{FileInput, LogArgs};
use rofi_voxel_reconfig::reconfig::all_next_worlds_not_norm;
use rofi_voxel_reconfig::voxel_world::impls::MapVoxelWorld;
use rofi_voxel_reconfig::voxel_world::normalized_eq_worlds;
use rofi_voxel_reconfig::voxel_world::NormVoxelWorld;
use rofi_voxel_reconfig::voxel_world::{check_voxel_world, is_normalized};
use std::assert_matches::assert_matches;

type IndexType = i8;

/// Compute one step of the reconfiguration algorithm.
///
/// Returns the (normalized) input world as the first world of the sequence
#[derive(Debug, Parser)]
struct Cli {
    /// World file in voxel-json format ('-' for standard input)
    world_file: String,
    /// Return result in a short json format
    #[arg(short, long, default_value_t = false)]
    short: bool,
    /// Don't warn when normalizing the input world
    #[arg(short, long, default_value_t = false)]
    normalize: bool,
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

fn get_next_worlds<TWorld: NormVoxelWorld>(world: &TWorld) -> Vec<TWorld>
where
    TWorld::IndexType: num::Integer + std::hash::Hash,
{
    assert_matches!(check_voxel_world(world), Ok(()));
    assert!(is_normalized(world));

    all_next_worlds_not_norm(world).collect()
}

fn main() -> Result<()> {
    let args = Cli::parse();

    args.log.setup_logging()?;

    let InputWorlds { world } = args.get_worlds()?;
    let (world, _min_pos) = world.to_world_and_min_pos::<MapVoxelWorld<_>>()?;
    assert_matches!(check_voxel_world(&world), Ok(()));

    let world = if !is_normalized(&world) {
        if !args.normalize {
            eprintln!("Normalizing init world");
        }
        normalized_eq_worlds(&world).next().unwrap()
    } else {
        world
    };

    let next_worlds = get_next_worlds(&world);

    let worlds = std::iter::once(&world)
        .chain(&next_worlds)
        .map(rofi_voxel_reconfig::serde::VoxelWorld::from_world)
        .collect::<Vec<_>>();

    if args.short {
        serde_json::to_writer(std::io::stdout(), &worlds)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &worlds)?;
    }

    Ok(())
}
