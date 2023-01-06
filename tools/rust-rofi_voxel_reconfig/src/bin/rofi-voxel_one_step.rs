#![feature(assert_matches)]

use anyhow::Result;
use clap::Parser;
use rofi_voxel::connectivity::ConnectivityGraph;
use rofi_voxel::reconfiguration::all_possible_next_worlds_not_norm;
use rofi_voxel::voxel_world::VoxelWorld;
use rofi_voxel_cli::FileInput;
use std::assert_matches::assert_matches;

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
}

#[derive(Debug)]
struct InputWorlds {
    world: rofi_voxel::serde::VoxelWorld,
}

impl Cli {
    pub fn get_worlds(&self) -> Result<InputWorlds> {
        let world = FileInput::from_arg(&self.world_file);
        let world = world.read_all()?;
        let world = serde_json::from_str(&world)?;

        Ok(InputWorlds { world })
    }
}

fn get_next_worlds(world: &VoxelWorld) -> Vec<VoxelWorld> {
    assert_matches!(world.check_voxel_world(), Ok(()));
    assert!(world.is_normalized());

    all_possible_next_worlds_not_norm(
        world,
        &ConnectivityGraph::compute_from(world),
        world.all_bodies().count(),
    )
    .collect()
}

fn main() -> Result<()> {
    let args = Cli::parse();

    let InputWorlds { world } = args.get_worlds()?;
    let (world, _min_pos) = world.to_world_and_min_pos()?;
    assert_matches!(world.check_voxel_world(), Ok(()));

    let world = if !world.is_normalized() {
        if !args.normalize {
            eprintln!("Normalizing init world");
        }
        world.normalized_eq_worlds().next().unwrap()
    } else {
        world
    };

    let next_worlds = get_next_worlds(&world);

    let worlds = std::iter::once(&world)
        .chain(&next_worlds)
        .map(rofi_voxel::serde::VoxelWorld::from_world)
        .collect::<Vec<_>>();

    if args.short {
        serde_json::to_writer(std::io::stdout(), &worlds)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &worlds)?;
    }

    Ok(())
}
