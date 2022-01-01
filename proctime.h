struct proctime
{
  int start_time;         // process arrival time
  int end_time;           // process exit or kill time
  int run_time;           // number of ticks the process was RUNNING in
  int wait_time;          // number of ticks the process was RUNNABLE in
  int sleep_time;         // number of ticks the process was SLEEPING in
  int n_context_switches; // number of context switches (how many times it entered the scheduler)
  int time_slice;         // for RR && MLFQ : remaining time slice until the next yield call (remaining quantum time)
};