#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
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
  int full_semid = semcreate(FULLKEY, BUFSIZE);
  int empty_semid = semcreate(EMPTYKEY, 0);
  int mutex_semid = semcreate(MUTEXKEY, 1);

  for (i = 0; i < NPRODS; i++)
  {
    if (fork() == 0)
    {
      // execute producer
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

  semclose(full_semid);
  semclose(empty_semid);
  semclose(mutex_semid);
  return 0;
}
