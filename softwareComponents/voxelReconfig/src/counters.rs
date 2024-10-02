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
pub struct Counters {
    pub sum: SuccessorsCallCounters,
    pub count: usize,
}

#[derive(Debug, Clone, serde::Serialize)]
pub struct CountersResult {
    pub total: Counters,
    pub bfs_layers: Option<Vec<Counters>>,
}

#[derive(Debug, Clone, serde::Serialize)]
pub struct SuccessorsCallCounters {
    pub modules: usize,
    pub successful_cuts: usize,
    pub failed_cuts: usize,
    pub duplicate_moves: usize,
    pub collided_moves: usize,
    pub new_unique_states: usize,
}

impl SuccessorsCallCounters {
    fn new() -> Self {
        Self {
            modules: 0,
            successful_cuts: 0,
            failed_cuts: 0,
            duplicate_moves: 0,
            collided_moves: 0,
            new_unique_states: 0,
        }
    }

    fn from_counter_logs<ICountersLogs>(logs: ICountersLogs) -> Self
    where
        ICountersLogs: IntoIterator<Item = CountersLog>,
    {
        let mut counters = SuccessorsCallCounters::new();
        let mut all_moves = 0;
        for log in logs {
            match log {
                CountersLog::NewSuccessorsCall => panic!("Unexpected value"),
                CountersLog::NewModule => counters.modules += 1,
                CountersLog::NewSuccessfulCut => counters.successful_cuts += 1,
                CountersLog::NewFailedCut => counters.failed_cuts += 1,
                CountersLog::NewMove => all_moves += 1,
                CountersLog::MoveCollided => counters.collided_moves += 1,
                CountersLog::SavedNewUniqueState => counters.new_unique_states += 1,
            }
        }
        counters.duplicate_moves = all_moves - counters.collided_moves - counters.new_unique_states;

        counters
    }

    fn add(&mut self, other: Self) {
        let Self {
            modules,
            successful_cuts,
            failed_cuts,
            duplicate_moves,
            collided_moves,
            new_unique_states,
        } = self;
        *modules += other.modules;
        *successful_cuts += other.successful_cuts;
        *failed_cuts += other.failed_cuts;
        *duplicate_moves += other.duplicate_moves;
        *collided_moves += other.collided_moves;
        *new_unique_states += other.new_unique_states;
    }
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

    fn get_bfs_layers(&self) -> Vec<Counters> {
        assert_eq!(self.logs[0], CountersLog::NewSuccessorsCall);
        let successors_calls = self.logs[1..]
            .split(|&log| log == CountersLog::NewSuccessorsCall)
            .map(|calls| SuccessorsCallCounters::from_counter_logs(calls.iter().copied()));

        let mut layers: Vec<Counters> = vec![];
        let mut current_layer = Counters {
            sum: SuccessorsCallCounters::new(),
            count: 0,
        };
        let mut in_current_layer_left = 1;
        let mut in_next_layer = 0;

        for call in successors_calls {
            if in_current_layer_left == 0 {
                assert!(in_next_layer > 0);
                layers.push(current_layer);
                current_layer = Counters {
                    sum: SuccessorsCallCounters::new(),
                    count: 0,
                };
                in_current_layer_left = in_next_layer;
                in_next_layer = 0;
            }
            in_current_layer_left -= 1;
            in_next_layer += call.new_unique_states;

            current_layer.sum.add(call);
            current_layer.count += 1;
        }
        layers.push(current_layer);

        layers
    }

    fn get_total(&self) -> Counters {
        let total_count = self
            .logs
            .iter()
            .filter(|&&log| log == CountersLog::NewSuccessorsCall)
            .count();

        let total_sum = SuccessorsCallCounters::from_counter_logs(
            self.logs
                .iter()
                .copied()
                .filter(|&log| log != CountersLog::NewSuccessorsCall),
        );
        Counters {
            sum: total_sum,
            count: total_count,
        }
    }

    pub fn get_results(bfs_layers: bool) -> CountersResult {
        let instance = INSTANCE
            .get()
            .expect("Counter has to be started before calling logs")
            .lock()
            .expect("Counter mutex failed while getting logs");

        let total = instance.get_total();
        let bfs_layers = if bfs_layers {
            Some(instance.get_bfs_layers())
        } else {
            None
        };

        CountersResult { total, bfs_layers }
    }
}
