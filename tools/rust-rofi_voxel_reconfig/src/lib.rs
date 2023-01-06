use anyhow::{ensure, Context, Result};
use std::fs::File;
use std::io::{BufReader, Read};

#[derive(Debug, Clone)]
pub enum FileInput {
    File(String),
    StdIn,
}

impl FileInput {
    pub fn from_arg<TString>(file_arg: TString) -> Self
    where
        TString: AsRef<str> + Into<String>,
    {
        match file_arg.as_ref() {
            "-" => Self::StdIn,
            _ => Self::File(file_arg.into()),
        }
    }

    pub fn get_reader(&self) -> Result<BufReader<Box<dyn Read>>> {
        match self {
            FileInput::File(path) => Ok(BufReader::new(Box::new(
                File::open(path).with_context(|| format!("Could not open file '{path}'"))?,
            ))),
            FileInput::StdIn => Ok(BufReader::new(Box::new(std::io::stdin()))),
        }
    }

    pub fn read_all(&self) -> Result<String> {
        let mut result = String::new();
        self.get_reader()?.read_to_string(&mut result)?;
        Ok(result)
    }

    pub fn check_max_one_stdin<'a, TInputs: IntoIterator<Item = &'a Self>>(
        inputs: TInputs,
    ) -> Result<()> {
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

#[derive(Debug, clap::Args)]
#[clap(mut_arg("verbose", |arg| arg.help_heading(Some("Logging"))))]
#[clap(mut_arg("quiet", |arg| arg.help_heading(Some("Logging"))))]
pub struct LogArgs {
    #[clap(flatten)]
    pub verbose: clap_verbosity_flag::Verbosity,
    /// Log file (if not specified, logs to stderr)
    #[arg(short, long("log"), help_heading = Some("Logging"))]
    pub log_file: Option<String>,
}

impl LogArgs {
    pub fn setup_logging(&self) -> std::io::Result<()> {
        match &self.log_file {
            Some(file_name) => {
                simple_logging::log_to_file(file_name, self.verbose.log_level_filter())?
            }
            None => simple_logging::log_to_stderr(self.verbose.log_level_filter()),
        }
        Ok(())
    }
}
