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

void increment(char[4]);
char inc(char c);

int main()
{
  // Open full and empty sems, and mutex
  int full_semid = semget(FULLKEY);
  int empty_semid = semget(EMPTYKEY);
  int mutex_semid = semget(MUTEXKEY);
  char numstr[4];

  for (int i = 0; i < NTASK; i++)
  {
    semwait(full_semid);
    semwait(mutex_semid);
    
    // Critic region
    int bufferd = open(BUFNAME, O_RDONLY);
    read(bufferd, numstr, 4);
    close(bufferd);

    increment(numstr);

    bufferd = open(BUFNAME, O_WRONLY | O_TRUNC);
    write(bufferd, numstr, 4);
    close(bufferd);

    printf("prod %d: %s\n", getpid(), numstr);

    semsignal(mutex_semid);
    semsignal(empty_semid);
  }

  //no need to close sems
}

void increment(char numstr[4])
{
  for (int i = 3; i >= 0; i--)
  {
    numstr[i] = inc(numstr[i]);
    if (numstr[i] != '0')
      break;
  }
}

char inc(char c)
{
  c++;
  if (c == '9' + 1)
  {
    c = '0';
  }
  return c;
}
