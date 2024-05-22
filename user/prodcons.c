#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "prodcons.h"

int main(int argc, char *argv[])
{
  int i;
  int bufferd;

  // Initialize the buffer file
  bufferd = open(BUFNAME, O_CREATE | O_WRONLY | O_TRUNC);
  write(bufferd, "0000\n", 5);
  close(bufferd);

  // Create full and empty sems, and mutex
  semcreate(FULLKEY, BUFSIZE);
  semcreate(EMPTYKEY, 0);
  semcreate(MUTEXKEY, 1);

  for (i = 0; i < NPRODS; i++)
  {
    if (fork() == 0)
    {
      // execute productor
      char *args[2] = {"prod", 0};
      int pid = exec("prod", args);
      fprintf(1, "prodcons: prod: not found\n", pid);
      return 0;
    }
  }

  for (i = 0; i < NCONS; i++)
  {
    if (fork() == 0)
    {
      // execute consumer
      char *args[2] = {"cons", 0};
      int pid = exec("cons", args);
      fprintf(1, "prodcons: cons: not found\n", pid);

      return 0;
    }
  }

  for (i = 0; i < NPRODS + NCONS; i++)
  {
    wait(0);
  }

  // no need to close sems
  return 0;
}
