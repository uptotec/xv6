#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
  int i, x = 0;
  for (i = 0; i < atoi(argv[1]); ++i)
    x += 1;

  exit();
}