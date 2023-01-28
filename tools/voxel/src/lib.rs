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
    /// Don't log to terminal (ERRORs will still be logged)
    #[arg(long, help_heading = Some("Logging"))]
    pub no_log_term: bool,
    /// Log to file in human readable format
    #[arg(long, help_heading = Some("Logging"))]
    pub log_file: Option<String>,
}

impl LogArgs {
    pub fn get_level(&self) -> log::LevelFilter {
        self.verbose.log_level_filter()
    }

    fn get_term_logger(
        mut log_level: log::LevelFilter,
        no_log_term: bool,
    ) -> Box<simplelog::TermLogger> {
        if no_log_term {
            log_level = std::cmp::min(log_level, log::LevelFilter::Error);
        }
        simplelog::TermLogger::new(
            log_level,
            simplelog::Config::default(),
            simplelog::TerminalMode::Stderr,
            simplelog::ColorChoice::Auto,
        )
    }

    fn get_file_logger(
        log_file: &str,
        log_level: log::LevelFilter,
    ) -> std::io::Result<Box<simplelog::WriteLogger<std::fs::File>>> {
        Ok(simplelog::WriteLogger::new(
            log_level,
            simplelog::Config::default(),
            File::options().append(true).create(true).open(log_file)?,
        ))
    }

    pub fn setup_logging(&self) -> Result<()> {
        let mut loggers = Vec::<Box<dyn simplelog::SharedLogger>>::new();

        loggers.push(Self::get_term_logger(self.get_level(), self.no_log_term));
        if let Some(log_file) = &self.log_file {
            loggers.push(Self::get_file_logger(log_file, self.get_level())?);
        }

        simplelog::CombinedLogger::init(loggers)?;
        Ok(())
    }
}
