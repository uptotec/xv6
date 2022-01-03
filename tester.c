#include "types.h"
#include "stat.h"
#include "user.h"

char *instructions =
    "\ntester [int][number of processes] [int][arrival delay] [int][number of runs]\n\n\
arrival time may have additional delay until the tester gets rescheduled to fork the next process\n\
number of proccess must be in range 1 <= number of proccess <= 99\n\
number of runs must be >= 1\n\n";

char *wrongarg = "\nwrong arguments\n";

char *startmessage =
    "\nTest started with number of processes = %d, arrival delay = %d and number of runs = %d\n\
\narrival time may have additional delay until the tester gets rescheduled to fork the next process\n";

char *proctable =
    "\n-------------------------------------------------------------------------------------------------------------------------------\n\
pid\t start\t\t running\t wating\t\t sleeping\t no. context switchs\t endTime\t estimate\n\
-------------------------------------------------------------------------------------------------------------------------------\n";

char *procrow = "%d\t %d\t\t %d\t\t %d\t\t %d\t\t %d\t\t\t %d\t\t %d\n";

char *newrow = "-------------------------------------------------------------------------------------------------------------------------------\n";

char *averageresults = "\naverage turnaround Time : %d, average wating Time : %d\n";

char *finished = "\ntest finished successfully\n";

int main(int argc, char *argv[])
{

  if (!strcmp(argv[1], "-h"))
  {
    printf(1, instructions);
    exit();
  }

  if (argc != 4)
  {
    printf(1, wrongarg);
    printf(1, instructions);
    exit();
  }

  if (atoi(argv[1]) < 1 && atoi(argv[1]) > 99)
  {
    printf(1, wrongarg);
    printf(1, instructions);
  }

  if (atoi(argv[3]) < 1)
  {
    printf(1, wrongarg);
    printf(1, instructions);
  }

  int nProc = atoi(argv[1]), delay = atoi(argv[2]), nRuns = atoi(argv[3]);

  int average_run, average_wait, average_sleep, runs = 1, i;
  int *proc = (int *)malloc(nProc * sizeof(int)), *pid = (int *)malloc(nProc * sizeof(int));
  struct proctime *time = (struct proctime *)malloc(nProc * sizeof(struct proctime));

  printf(1, startmessage, nProc, delay, nRuns);

rerun:

  average_run = 0;
  average_wait = 0;
  average_sleep = 0;

  printf(1, "\nrun no.%d\n", runs);
  for (i = 0; i < nProc; i++)
  {
    char *forkname = "tester  ";

    *(forkname + 6) = ((i + 1) / 10) + 48;
    *(forkname + 7) = ((i + 1) % 10) + 48;

    if ((proc[i] = forkandrename(forkname)) == 0)
    {
      int x, y = 0;
      if (i < (nProc / 2))
      {
        for (x = 0; x < (atoi("5000000") * (i + 1)); x++)
          y += x;
      }
      else
      {
        for (x = 0; x < (atoi("5000000") * (nProc - i)); x++)
          y += x;
      }
      exit();
    }
    sleep(delay);
  }

  for (i = 0; i < nProc; i++)
  {
    pid[i] = waitandgettime(&time[i]);
  }

  printf(1, proctable);

  for (i = 0; i < nProc; i++)
  {
    printf(1, procrow,
           pid[i],
           time[i].start_time,
           time[i].run_time,
           time[i].wait_time,
           time[i].sleep_time,
           time[i].n_context_switches,
           time[i].end_time,
           time[i].predicted_time);

    printf(1, newrow);

    average_run += time[i].run_time;
    average_wait += time[i].wait_time;
    average_sleep += time[i].sleep_time;
  }

  average_run /= nProc;
  average_wait /= nProc;
  average_sleep /= nProc;
  printf(1, averageresults, average_run + average_wait + average_sleep, average_wait);

  nRuns--;
  runs++;
  if (nRuns)
    goto rerun;

  printf(1, finished);
  exit();
}