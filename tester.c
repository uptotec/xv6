#include "types.h"
#include "stat.h"
#include "user.h"

void swap(int *xp, int *yp)
{
  int temp = *xp;
  *xp = *yp;
  *yp = temp;
}

void swapproctime(struct proctime *xp, struct proctime *yp)
{
  struct proctime temp = *xp;
  *xp = *yp;
  *yp = temp;
}

// from https://www.geeksforgeeks.org/c-program-to-sort-an-array-in-ascending-order/
// Function to perform Selection Sort
void selectionSort(int *pid, struct proctime *time, int n)
{
  int i, j, min_idx;

  // One by one move boundary of unsorted subarray
  for (i = 0; i < n - 1; i++)
  {

    // Find the minimum element in unsorted array
    min_idx = i;
    for (j = i + 1; j < n; j++)
      if (pid[j] < pid[min_idx])
        min_idx = j;

    // Swap the found minimum element
    // with the first element
    swap(&pid[min_idx], &pid[i]);
    swapproctime(&time[min_idx], &time[i]);
  }
}

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

  int average_run, average_wait, average_sleep, runs = 1, i;
  int *proc = (int *)malloc(nProc * sizeof(int)), *pid = (int *)malloc(nProc * sizeof(int));
  struct proctime *time = (struct proctime *)malloc(nProc * sizeof(struct proctime));

  printf(1, "\nTest started with number of processes = %d, arrival delay = %d and number of runs = %d\n", nProc, delay, nRuns);
  printf(1, "\narrival time may have additional delay until the tester gets rescheduled to fork the next process\n");

rerun:

  average_run = 0;
  average_wait = 0;
  average_sleep = 0;

  printf(1, "\nrun no.%d\n", runs);
  for (i = 0; i < nProc; i++)
  {
    char *test = "tester ";
    *(test + 6) = i + 49;
    if ((proc[i] = forkandrename(test)) == 0)
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

  // selectionSort(pid, time, nProc);

  printf(1, "\npid\t start\t running\t wating\t sleeping\t no. context switchs\t endTime\t estimate\n");
  for (i = 0; i < nProc; i++)
  {
    printf(1, "%d\t %d\t %d\t\t %d\t %d\t\t %d\t\t\t %d\t\t %d\n",
           pid[i],
           time[i].start_time,
           time[i].run_time,
           time[i].wait_time,
           time[i].sleep_time,
           time[i].n_context_switches,
           time[i].end_time, time[i].predicted_time);

    average_run += time[i].run_time;
    average_wait += time[i].wait_time;
    average_sleep += time[i].sleep_time;
  }

  average_run /= nProc;
  average_wait /= nProc;
  average_sleep /= nProc;
  printf(1, "\naverage turnaround Time : %d, average wating Time : %d\n", average_run + average_wait + average_sleep, average_wait);

  nRuns--;
  runs++;
  if (nRuns)
    goto rerun;

  printf(1, "\ntest finished successfully\n");
  exit();
}