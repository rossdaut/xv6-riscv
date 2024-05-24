#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
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
    
    /* Enter critical region */

    // Read buffer content
    int bufferd = open(BUFNAME, O_RDONLY);
    read(bufferd, numstr, 4);
    close(bufferd);

    decrement(numstr);

    // Write new buffer content
    bufferd = open(BUFNAME, O_WRONLY | O_TRUNC);
    write(bufferd, numstr, 4);
    close(bufferd);

    printf("\t\t\tcons %d: %s\n", getpid(), numstr);
    
    /* Leave critical region */

    semsignal(mutex_semid);
    semsignal(full_semid);
  }
  
  semclose(full_semid);
  semclose(empty_semid);
  semclose(mutex_semid);
  return 0;
}

// Decrement by one the number represented by the string
// Input string must be a four digit number
// The decrement is done in place
void decrement(char numstr[4])
{
  for (int i = 3; i >= 0; i--)
  {
    numstr[i] = dec(numstr[i]);
    if (numstr[i] != '9')
      break;
  }
}

// Decrement the digit by one
// If the digit was 0, set 9
// Input char must be a digit
char dec(char c)
{
  c--;
  if (c == '0' - 1)
  {
    c = '9';
  }
  return c;
}
