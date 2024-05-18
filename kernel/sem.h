#include "spinlock.h"
#include "sleeplock.h"

struct semaphore {
  int key;
  int value;
  int refcount;
  struct sleeplock sleeplock;
  struct spinlock lock;
};

// Preguntar si poner acá tmb los perfiles