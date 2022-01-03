enum progstate
{
  EMPTY,
  USED
};

struct proghistory
{
  char name[16];           // name of the program
  enum progstate state;    // is this slot used
  int predicted_time;      // time prediction for the next cpu burst
  int last_predicted_time; // last prediction for the cpu burst
  int last_run_time;       // last number of ticks the process was RUNNING in
};