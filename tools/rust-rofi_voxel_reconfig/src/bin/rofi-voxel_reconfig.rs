use clap::Parser;
use failure::Error;
use input::Input;
use rofi_voxel::reconfiguration;

pub mod input {
    use failure::{ensure, Error, ResultExt};
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

        pub fn check_max_one_stdin<'a, TInputs: IntoIterator<Item = &'a Self>>(
            inputs: TInputs,
        ) -> Result<(), Error> {
            ensure!(
                inputs
                    .into_iter()
                    .filter(|&input| matches!(input, Self::StdIn))
                    .count()
                    <= 1,
                "Cannot redirect multiple files to stdin"
            );
            Ok(())
        }
    }
}

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
}

#[derive(Debug)]
struct InputWorlds {
    init: rofi_voxel::serde::VoxelWorld,
    goal: rofi_voxel::serde::VoxelWorld,
}

impl Cli {
    pub fn get_worlds(&self) -> Result<InputWorlds, Error> {
        let init = Input::from_arg(&self.init_world_file);
        let goal = Input::from_arg(&self.goal_world_file);

        Input::check_max_one_stdin([&init, &goal])?;

        let init = init.read_all()?;
        let goal = goal.read_all()?;

        let init = serde_json::from_str(&init)?;
        let goal = serde_json::from_str(&goal)?;

        Ok(InputWorlds { init, goal })
    }
}

fn main() -> Result<(), Error> {
    let args = Cli::parse();

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
