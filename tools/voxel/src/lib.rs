use anyhow::{ensure, Context, Result};
use std::fs::File;
use std::io::{BufReader, Read};

pub use self::logging::LogArgs;

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

pub mod logging {
    use anyhow::Result;

    #[derive(Debug, clap::Args)]
    #[clap(mut_arg("verbose", |arg| arg.help_heading(Some("Logging"))))]
    #[clap(mut_arg("quiet", |arg| arg.help_heading(Some("Logging"))))]
    pub struct LogArgs {
        #[clap(flatten)]
        pub verbose: clap_verbosity_flag::Verbosity,
        /// Don't log to terminal (Errors will still be logged)
        #[arg(long, help_heading = Some("Logging"))]
        pub no_log_term: bool,
        /// Log to file in human readable format
        #[arg(long, help_heading = Some("Logging"))]
        pub log_file: Option<String>,
        /// Log to file in JSON array format (each message is a single json object)
        #[arg(long, help_heading = Some("Logging"))]
        pub log_json: Option<String>,
    }

    impl LogArgs {
        pub fn get_level(&self) -> log::LevelFilter {
            self.verbose.log_level_filter()
        }

        pub fn get_term_level(self) -> log::LevelFilter {
            if self.no_log_term {
                std::cmp::min(self.get_level(), log::LevelFilter::Error)
            } else {
                self.get_level()
            }
        }

        fn term_encoder() -> Box<impl log4rs::encode::Encode> {
            let pattern = "[{date(%H:%M:%S)} {h({level:5.5})} {target}] {message}{n}";
            Box::new(log4rs::encode::pattern::PatternEncoder::new(pattern))
        }

        fn file_encoder() -> Box<impl log4rs::encode::Encode> {
            let pattern = "[{date(%H:%M:%S)} {level:5.5} {target}] {message}{n}";
            Box::new(log4rs::encode::pattern::PatternEncoder::new(pattern))
        }

        fn json_encoder() -> Box<impl log4rs::encode::Encode> {
            Box::new(log4rs::encode::json::JsonEncoder::new())
        }

        fn get_term_appender(name: &str, no_log_term: bool) -> log4rs::config::Appender {
            let appender = Box::new(
                log4rs::append::console::ConsoleAppender::builder()
                    .target(log4rs::append::console::Target::Stderr)
                    .encoder(Self::term_encoder())
                    .build(),
            );
            if no_log_term {
                let filter =
                    log4rs::filter::threshold::ThresholdFilter::new(log::LevelFilter::Error);
                log4rs::config::Appender::builder()
                    .filter(Box::new(filter))
                    .build(name, appender)
            } else {
                log4rs::config::Appender::builder().build(name, appender)
            }
        }

        fn get_file_appender(
            name: &str,
            log_file: &str,
            encoder: Box<dyn log4rs::encode::Encode>,
        ) -> std::io::Result<log4rs::config::Appender> {
            Ok(log4rs::config::Appender::builder().build(
                name,
                Box::new(
                    log4rs::append::file::FileAppender::builder()
                        .encoder(encoder)
                        .build(log_file)?,
                ),
            ))
        }

        pub fn setup_logging(&self) -> Result<log4rs::Handle> {
            let mut appenders = vec![Self::get_term_appender("term", self.no_log_term)];

            if let Some(log_file) = &self.log_file {
                appenders.push(Self::get_file_appender(
                    "file",
                    log_file,
                    Self::file_encoder(),
                )?);
            }
            if let Some(json_log_file) = &self.log_json {
                appenders.push(Self::get_file_appender(
                    "json",
                    json_log_file,
                    Self::json_encoder(),
                )?);
            }

            let root = log4rs::config::Root::builder()
                .appenders(appenders.iter().map(|appender| appender.name()))
                .build(self.get_level());
            let config = log4rs::Config::builder().appenders(appenders).build(root)?;

            Ok(log4rs::init_config(config)?)
        }
    }
}
