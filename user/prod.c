#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "prodcons.h"

void increment(char[4]);
char inc(char c);

int main()
{
  int shmid;
  struct buffer *buf;
  int counter = 0;
  
  shmid = shmget(BUFKEY, sizeof(struct buffer), (void *) &buf);
  if (shmid == 0) {
    fprintf(1, "prod: shmget failed\n");
    exit(1);
  }

  // Open full and empty sems, and mutex
  int full_semid = semget(FULLKEY);
  int empty_semid = semget(EMPTYKEY);
  int mutex_semid = semget(MUTEXKEY);

  for (int i = 0; i < NTASK; i++)
  {
    semwait(full_semid);
    semwait(mutex_semid);
    
    /* Enter critical region */

    // Enqueue
    buf->data[buf->tail] = counter;
    buf->tail = (buf->tail + 1) % BUFSIZE;
    buf->size++;

    printf("prod %d: %d\n", getpid(), counter++);
    
    /* Leave critical region */

    semsignal(mutex_semid);
    semsignal(empty_semid);
  }

  semclose(full_semid);
  semclose(empty_semid);
  semclose(mutex_semid);
  return 0;
}