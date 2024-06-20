#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "prodcons.h"

/* The console output should be interpreted as follows:

 prod <pid>: <buf content>
                         cons <pid>: <buf content>

 where <buf content> is the content of the file after the
 prod/cons operation */
int main(int argc, char *argv[])
{
  int i, shmid;
  struct buffer *buf;

  shmid = shmget(BUFKEY, sizeof(struct buffer), (void *) &buf);
  if (shmid == 0) {
    fprintf(1, "prodcons: shmget failed\n");
    exit(1);
  }

  printf("prodcons: buffer created\n");
  buf->head = 0;
  buf->tail = 0;
  buf->size = 0;

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