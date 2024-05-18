#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "sem.h"
#include "types.h"

uint64 sys_semcreate() {
  int key, initvalue;

  argint(0, &key);
  argint(1, &initvalue);
  
  return semcreate(key, initvalue);
}

uint64 sys_semget() {
  int key;

  argint(0, &key);
  return semget(key);
}

uint64 sys_semwait() {
  int semid;
  
  argint(0, &semid);
  return semwait(semid);
}

uint64 sys_semsignal() {
  int semid;

  argint(0, &semid);
  return semsignal(semid);
}

uint64 sys_semclose() {
  int semid;

  argint(0, &semid);
  return semclose(semid);
}
