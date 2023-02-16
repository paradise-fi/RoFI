use std::sync::{Mutex, OnceLock};

#[derive(Debug, Clone, Copy, amplify::Display, amplify::Error)]
#[display(doc_comments)]
pub enum Error {
    /// Counter already started
    AlreadyStarted,
    /// Counter not started
    NotStarted,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CountersLog {
    NewSuccessorsCall,
    NewModule,
    NewSuccessfulCut,
    NewFailedCut,
    NewMove,
    MoveCollided,
    SavedNewUniqueState,
}

pub struct Counter {
    logs: Vec<CountersLog>,
}
static INSTANCE: OnceLock<Mutex<Counter>> = OnceLock::new();

#[derive(Debug, Clone, serde::Serialize)]
pub struct CountersResult {
    pub successors_calls: Vec<SuccessorsCallCounters>,
}

#[derive(Debug, Clone, serde::Serialize)]
pub struct SuccessorsCallCounters {
    pub modules: usize,
    pub successful_cuts: usize,
    pub failed_cuts: usize,
    pub all_moves: usize,
    pub duplicate_moves: usize,
    pub collided_moves: usize,
    pub new_unique_states: usize,
}

impl Counter {
    pub fn start() -> Result<(), Error> {
        INSTANCE
            .set(Mutex::new(Counter { logs: Vec::new() }))
            .map_err(|_| Error::AlreadyStarted)
    }

    pub fn get_logs() -> Vec<CountersLog> {
        INSTANCE
            .get()
            .expect("Counter has to be started before calling logs")
            .lock()
            .expect("Counter mutex failed while getting logs")
            .logs
            .clone()
    }

    fn new_log(log: CountersLog) {
        if let Some(global) = INSTANCE.get() {
            global.lock().expect("Counter mutex failed").logs.push(log);
        }
    }

    pub fn new_successors_call() {
        Self::new_log(CountersLog::NewSuccessorsCall)
    }
    pub fn new_module() {
        Self::new_log(CountersLog::NewModule)
    }
    pub fn new_successful_cut() {
        Self::new_log(CountersLog::NewSuccessfulCut)
    }
    pub fn new_failed_cut() {
        Self::new_log(CountersLog::NewFailedCut)
    }
    pub fn new_move() {
        Self::new_log(CountersLog::NewMove)
    }
    pub fn move_collided() {
        Self::new_log(CountersLog::MoveCollided)
    }
    pub fn saved_new_unique_state() {
        Self::new_log(CountersLog::SavedNewUniqueState)
    }

    pub fn get_results() -> CountersResult {
        let instance = INSTANCE
            .get()
            .expect("Counter has to be started before calling logs")
            .lock()
            .expect("Counter mutex failed while getting logs");

        assert_eq!(instance.logs[0], CountersLog::NewSuccessorsCall);
        let successors_calls = instance.logs[1..]
            .split(|&log| log == CountersLog::NewSuccessorsCall)
            .map(Self::get_successors_call_result)
            .collect::<Vec<_>>();

        CountersResult { successors_calls }
    }

    fn get_successors_call_result(logs: &[CountersLog]) -> SuccessorsCallCounters {
        let mut modules = 0;
        let mut successful_cuts = 0;
        let mut failed_cuts = 0;
        let mut all_moves = 0;
        let mut collided_moves = 0;
        let mut new_unique_states = 0;
        for log in logs {
            match log {
                CountersLog::NewSuccessorsCall => panic!("Unexpected value"),
                CountersLog::NewModule => modules += 1,
                CountersLog::NewSuccessfulCut => successful_cuts += 1,
                CountersLog::NewFailedCut => failed_cuts += 1,
                CountersLog::NewMove => all_moves += 1,
                CountersLog::MoveCollided => collided_moves += 1,
                CountersLog::SavedNewUniqueState => new_unique_states += 1,
            }
        }

        let duplicate_moves = all_moves - collided_moves - new_unique_states;
        SuccessorsCallCounters {
            modules,
            successful_cuts,
            failed_cuts,
            all_moves,
            duplicate_moves,
            collided_moves,
            new_unique_states,
        }
    }
}
