#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "prodcons.h"

void decrement(char[4]);
char dec(char c);

int main()
{
  int shmid, prodid, content;
  struct buffer *buf;
  
  shmid = shmget(BUFKEY, sizeof(struct buffer), (void *) &buf);
  if (shmid < 0) {
    fprintf(1, "cons: shmget failed\n");
    exit(1);
  }

  // Open full and empty sems, and mutex
  int full_semid = semget(FULLKEY);
  int empty_semid = semget(EMPTYKEY);
  int mutex_semid = semget(MUTEXKEY);

  for (int i = 0; i < NTASK; i++) {
    // sleep(3);
    semwait(empty_semid);
    semwait(mutex_semid);
    
    /* Enter critical region */

    // Read buffer content
    prodid = buf->data[buf->head][0];
    content = buf->data[buf->head][1];
    buf->head = (buf->head + 1) % BUFSIZE;
    buf->size--;

    printf("\t\t\tcons %d: [%d, %d]\n", getpid(), prodid, content);
    
    /* Leave critical region */

    semsignal(mutex_semid);
    semsignal(full_semid);
  }
  
  semclose(full_semid);
  semclose(empty_semid);
  semclose(mutex_semid);

  shmclose(shmid);
  return 0;
}