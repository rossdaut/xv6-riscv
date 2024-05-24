#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
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
    
    /* Enter critical region */

    // Read buffer content
    int bufferd = open(BUFNAME, O_RDONLY);
    read(bufferd, numstr, 4);
    close(bufferd);

    increment(numstr);

    // Write new buffer content
    bufferd = open(BUFNAME, O_WRONLY | O_TRUNC);
    write(bufferd, numstr, 4);
    close(bufferd);

    printf("prod %d: %s\n", getpid(), numstr);
    
    /* Leave critical region */

    semsignal(mutex_semid);
    semsignal(empty_semid);
  }

  semclose(full_semid);
  semclose(empty_semid);
  semclose(mutex_semid);
  return 0;
}

// Increment by one the number represented by the string
// Input string must be a four digit number
// The increment is done in place
void increment(char numstr[4])
{
  for (int i = 3; i >= 0; i--)
  {
    numstr[i] = inc(numstr[i]);
    if (numstr[i] != '0')
      break;
  }
}

// Increment the digit by one
// If the digit was 9, go back to 0
// Input char must be a digit
char inc(char c)
{
  c++;
  if (c == '9' + 1)
  {
    c = '0';
  }
  return c;
}
