#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{

  if (!strcmp(argv[1], "-h"))
  {
    printf(1, "\ntester [int][number of processes] [int][arrival delay] [int][number of runs] [0 same][1 random]\n");
    printf(1, "\narrival time may have additional delay until the tester gets rescheduled to fork the next process\n");
    exit();
  }

  if (argc != 4)
  {
    printf(1, "\nwrong arguments\n");
    printf(1, "\ntester [int][number of processes] [int][arrival delay] [int][number of runs] [0 same][1 random]\n");
    printf(1, "\narrival time may have additional delay until the tester gets rescheduled to fork the next process\n\n");
    exit();
  }

  if (atoi(argv[1]) < 1)
  {
    printf(1, "\nwrong arguments\n");
    printf(1, "number of proccess must be more than or equal 1\n");
  }

  if (atoi(argv[3]) < 1)
  {
    printf(1, "\nwrong arguments\n");
    printf(1, "number of runs must be more than or equal 1\n");
  }

  int nProc = atoi(argv[1]), delay = atoi(argv[2]), nRuns = atoi(argv[3]);

  int average_run, average_wait, average_sleep, i;
  int *proc = (int *)malloc(nProc * sizeof(int)), *pid = (int *)malloc(nProc * sizeof(int));
  struct proctime *time = (struct proctime *)malloc(nProc * sizeof(struct proctime));

  printf(1, "\nTest started with number of processes = %d, arrival delay = %d and number of runs = %d\n", nProc, delay, nRuns);
  printf(1, "\narrival time may have additional delay until the tester gets rescheduled to fork the next process\n\n");

rerun:

  average_run = 0;
  average_wait = 0;
  average_sleep = 0;

  for (i = 0; i < nProc; i++)
  {
    if ((proc[i] = fork()) == 0)
    {
      int x, y = 0;
      for (x = 0; x < atoi("10000000"); x++)
        y += x;
      exit();
    }
    sleep(delay);
  }

  for (i = 0; i < nProc; i++)
  {
    pid[i] = waitandgettime(&time[i]);
  }

  for (i = 0; i < nProc; i++)
  {
    printf(1, "pid: %d start: %d, running: %d, wating: %d, sleeping: %d, number of context switch: %d, endTime: %d\n",
           pid[i],
           time[i].start_time,
           time[i].run_time,
           time[i].wait_time,
           time[i].sleep_time,
           time[i].n_context_switches,
           time[i].end_time);

    average_run += time[i].run_time;
    average_wait += time[i].wait_time;
    average_sleep += time[i].sleep_time;
  }

  average_run /= nProc;
  average_wait /= nProc;
  average_sleep /= nProc;
  printf(1, "\naverage turnaround Time : %d, average wating Time : %d\n", average_run + average_wait + average_sleep, average_wait);

  nRuns--;
  if (nRuns)
    goto rerun;

  printf(1, "\ntest finished successfully\n");
  exit();
}