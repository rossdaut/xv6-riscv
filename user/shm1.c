#include "kernel/types.h"
#include "user/user.h"
#include "user/shm.h"

#define CHILDREN 1

int main() {
  int *arr;
  int i;

  if (shmget(SHMKEY, sizeof(int) * ARRSIZE, (void **) &arr) == -1) {
    printf("shm1: shmget failed\n");
    exit(-1);
  }

  for (i = 0; i < ARRSIZE; i++) {
    arr[i] = i*2;
  }

  for(i=0; i<CHILDREN; i++){
    if (fork() == 0) {
      char *args[2] = {"shm2", 0};
      int pid = exec("shm2", args);
      fprintf(1, "shm1: shm2 exec failed\n", pid);
      return 0;
    }
  }
  
  for(i=0; i<CHILDREN; i++) {
    wait(0);
  }
  
  return 0;
}
