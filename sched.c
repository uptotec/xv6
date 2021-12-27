#include "types.h"
#include "user.h"

int main(int argc, char *argv[])
{
#if defined(SCHEDULER_RR)
  printf(1, "Round Robin Scheduler\n");
#elif defined(SCHEDULER_SJF)
  printf(1, "Shortest Job First Scheduler\n");
#elif defined(SCHEDULER_MLFQ)
  printf(1, "Multi-Level Feedback Queue Scheduler\n");
#endif

  exit();
}
