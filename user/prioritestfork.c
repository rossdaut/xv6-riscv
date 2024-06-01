#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int priority, i, k, x = 0;

  for (priority = 1; priority < NLEVELS; priority++) {
    for (i = 0; i < 3; i++) {
      if (fork() == 0) {
        for(k = 0; k<100000; k++){
          set_priority(priority - 1);
          x++;
        }
        exit(0);
      }
    }
  }

  fprintf(2,"parent: %d\n", getpid());

  for(i = 0; i<9; i++){
    int pid = wait(0);
    fprintf(2, "%d\n", pid);
  }

  return 0;
}
