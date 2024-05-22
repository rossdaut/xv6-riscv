#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "proc.h"

struct semaphore {
  int key;
  int value;
  int refcount;
  struct spinlock lock;
};

struct semaphore sems[NSEM];

// Allocate a semaphore structure
// Search for a semaphore in sems[NSEM] with s->refcount equal to 0
// If found, return sempahore pointer with s->lock held.
// If there are no free sems, return 0.
void seminit() {
  struct semaphore *s;
  
  for (s = sems; s < &sems[NSEM]; s++) {
      initlock(&s->lock, "sem");
      s->key = -1;
      s->value = s->refcount = 0;
  }
}

// Allocate a semaphore structure
struct semaphore *semalloc() {
  struct semaphore *s;

  for (s = sems; s < &sems[NSEM]; s++) {
    acquire(&s->lock);
    if (s->refcount == 0) {
      return s;
    }
    release(&s->lock);
  }

  return 0;
}

// Allocate a semaphore pointer in the process' osem array
int procsemalloc(struct semaphore *s) {
  struct proc *p = myproc();
  int sd;

  // no hace falta adquirir el lock del proceso ¿Verdad?
  for (sd = 0; sd < NOSEM; sd++) {
    if (p->osem[sd] == 0) {
      p->osem[sd] = s;
      return sd;
    }
  }

  return -1;
}

// Check if a semaphore with the given key exists in sems[NSEM]
// If found, return sempahore pointer with s->lock held.
// Otherwise, return 0.
struct semaphore *findkey(int key) {
  struct semaphore *s;

  for (s = sems; s < &sems[NSEM]; s++) {
    acquire(&s->lock);
    if (s->key == key) {
      return s;
    }
    release(&s->lock);
  }

  return 0;
}

int keyexists(int key) {
  struct semaphore *s;
  if ((s = findkey(key)) != 0) {
    release(&s->lock);
    return 1;
  }
  return 0;
}

int semcreate(int key, int initvalue) {
  struct semaphore *s;
  int semid;

  //puede haber conflictos con el s->lock (se ejecuta der antes de que termine izq) ???  
  if (keyexists(key) || (s = semalloc()) == 0) {
    return -1;
  }

  if ((semid = procsemalloc(s)) < 0) {
    return -1;
  }

  s->key = key;
  s->value = initvalue;
  s->refcount = 1;
  
  release(&s->lock);
  return semid;
}

int semget(int key) {
  struct semaphore *s;
  int semid;

  if ((s = findkey(key)) == 0) {
    return -1;
  }

  if ((semid = procsemalloc(s)) < 0) {
    return -1;
  }

  s->refcount++;
  release(&s->lock);

  return semid;
}

struct semaphore *procfindsem(int semid) {
  struct proc *p = myproc();

  if (semid < 0 || semid >= NOFILE) {
    return 0;
  }

  return p->osem[semid];
}

int semwait(int semid) {
  struct semaphore *s;

  if ((s = procfindsem(semid)) == 0) {
    return -1;
  }
  
  acquire(&s->lock);
  while (s->value == 0) {
    sleep(s, &s->lock);
  }
   
  s->value--;
  release(&s->lock);

  return 0;
}

int semsignal(int semid) {
  struct semaphore *s;
  
  if ((s = procfindsem(semid)) == 0) {
    return -1;
  }

  acquire(&s->lock);
  s->value++;
  wakeup(s);
  release(&s->lock);

  return 0;
}

int semclose(int semid) {
  struct semaphore *s;
  struct proc *p = myproc();
  
  if ((s = procfindsem(semid)) == 0) {
    return -1;
  }

  acquire(&s->lock);
  p->osem[semid] = 0;
  if (--(s->refcount) == 0) {
    s->key = -1;
    s->value = 0;
  }
  release(&s->lock);
  
  return 0;
}