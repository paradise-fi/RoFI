#![feature(assert_matches)]

use clap::Parser;
use failure::Error;
use input::Input;
use itertools::Itertools;
use std::assert_matches::assert_matches;

mod input {
    use failure::{Error, ResultExt};
    use std::fs::File;
    use std::io::{BufReader, Read};

    #[derive(Debug, Clone)]
    pub enum Input {
        File(String),
        StdIn,
    }
    impl Input {
        pub fn from_arg<TString: AsRef<str> + Into<String>>(file_arg: TString) -> Self {
            match file_arg.as_ref() {
                "-" => Self::StdIn,
                _ => Self::File(file_arg.into()),
            }
        }

        pub fn get_reader(&self) -> Result<BufReader<Box<dyn std::io::Read>>, Error> {
            match self {
                Input::File(path) => Ok(BufReader::new(Box::new(
                    File::open(path).with_context(|_| format!("Could not open file {path:?}"))?,
                ))),
                Input::StdIn => Ok(BufReader::new(Box::new(std::io::stdin()))),
            }
        }

        pub fn read_all(&self) -> Result<String, Error> {
            let mut result = String::new();
            self.get_reader()?.read_to_string(&mut result)?;
            Ok(result)
        }
    }
}

/// Compute (unique) normalized versions of voxel world
#[derive(Debug, Parser)]
struct Cli {
    /// World file in voxel-json format ('-' for standard input)
    world_file: String,
    /// Return result in a short json format
    #[arg(short, long, default_value_t = false)]
    short: bool,
}

#[derive(Debug)]
struct InputWorlds {
    world: rofi_voxel::serde::VoxelWorld,
}

impl Cli {
    pub fn get_worlds(&self) -> Result<InputWorlds, Error> {
        let world = Input::from_arg(&self.world_file);
        let world = world.read_all()?;
        let world = serde_json::from_str(&world)?;

        Ok(InputWorlds { world })
    }
}

fn main() -> Result<(), Error> {
    let args = Cli::parse();

    let InputWorlds { world } = args.get_worlds()?;
    let (world, _min_pos) = world.to_world_and_min_pos()?;
    assert_matches!(world.check_voxel_world(), Ok(()));

    let norm_worlds = world
        .normalized_eq_worlds()
        .unique()
        .map(|eq_world| rofi_voxel::serde::VoxelWorld::from_world(&eq_world))
        .collect::<Vec<_>>();

    if args.short {
        serde_json::to_writer(std::io::stdout(), &norm_worlds)?;
    } else {
        serde_json::to_writer_pretty(std::io::stdout(), &norm_worlds)?;
    }

    Ok(())
}
