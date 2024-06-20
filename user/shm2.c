#include "kernel/types.h"
#include "user/user.h"
#include "user/shm.h"

int main(int argc, char const *argv[])
{
  int *arr;

  if (shmget(SHMKEY, sizeof(int) * ARRSIZE, (void **)&arr) == -1) {
    printf("shm2: shmget failed\n");
    exit(-1);
  }

  for (int i = 0; i < ARRSIZE; i++) {
    printf("shm2: arr[%d] = %d\n", i, arr[i]);
  }

  return 0;
}
