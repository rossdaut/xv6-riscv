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

void decrement(char[4]);
char dec(char c);

int main()
{
  // Open full and empty sems, and mutex
  int full_semid = semget(FULLKEY);
  int empty_semid = semget(EMPTYKEY);
  int mutex_semid = semget(MUTEXKEY);
  char numstr[4];

  for (int i = 0; i < NTASK; i++) {
    // sleep(3);
    semwait(empty_semid);
    semwait(mutex_semid);

    // Critic region
    int bufferd = open(BUFNAME, O_RDONLY);
    read(bufferd, numstr, 4);
    close(bufferd);

    decrement(numstr);

    bufferd = open(BUFNAME, O_WRONLY | O_TRUNC);
    write(bufferd, numstr, 4);
    close(bufferd);

    printf("\t\t\tcons %d: %s\n", getpid(), numstr);

    semsignal(mutex_semid);
    semsignal(full_semid);
  }
  //no need to close sems
}

void decrement(char numstr[4])
{
  for (int i = 3; i >= 0; i--)
  {
    numstr[i] = dec(numstr[i]);
    if (numstr[i] != '9')
      break;
  }
}

char dec(char c)
{
  c--;
  if (c == '0' - 1)
  {
    c = '9';
  }
  return c;
}
