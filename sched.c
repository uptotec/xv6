#include "types.h"
#include "user.h"

char *getalfa(void)
{
#ifdef ALFA

  if (ALFA == 1)
    return "0";
  else if (ALFA == 2)
    return "0.3";
  else if (ALFA == 3)
    return "0.5";
  else if (ALFA == 4)
    return "0.7";
  else if (ALFA == 5)
    return "1";

#endif
  return "0.5";
}

int main(int argc, char *argv[])
{
#if defined(SCHEDULER_RR) && defined(RR0)
  printf(1, "Round Robin Scheduler with quantum time = %d\n" RR0);
#elif defined(SCHEDULER_SJF) && defined(SJFDEF) && defined(ALFA)
  printf(1, "Shortest Job First Scheduler with default estimated burst time = %d and smoothing factor(α) = %s\n", SJFDEF, getalfa());
#elif defined(SCHEDULER_MLFQ) && defined(MLFQ0) && defined(MLFQ1) && defined(MLFQ2)
  printf(1, "Multi-Level Feedback Queue Scheduler with queue1 quantum time = %d, queue2 quantum time = %d and queue3 quantum time = %d\n", MLFQ0, MLFQ1, MLFQ2);
#endif

  exit();
}
