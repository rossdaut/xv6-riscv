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

// Initialize the semaphore table
// For each one, initialize its lock and variables
// This function should be run at system init (main.c)
void seminit() {
  struct semaphore *s;
  
  for (s = sems; s < &sems[NSEM]; s++) {
      initlock(&s->lock, "sem");
      s->key = -1;
      s->value = s->refcount = 0;
  }
}

// Allocate a semaphore structure
// Search for a free semaphore in the sems table with refcount equal to 0
// If found, return the sempahore pointer with its lock held.
// If there are no free sems, return 0.
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

// Allocate a semaphore pointer in the process' sem table
// Return the index in the table
// Return -1 if the table is full
int procsemalloc(struct semaphore *s) {
  struct proc *p = myproc();
  int sd;

  for (sd = 0; sd < NOSEM; sd++) {
    if (p->osem[sd] == 0) {
      p->osem[sd] = s;
      return sd;
    }
  }

  return -1;
}

// Find a semaphore with the given key in the sems table
// If found, return sempahore pointer with its lock held.
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

// Return 1 if a semaphore with the given key exists in the sems table
// Return 0 otherwise
int keyexists(int key) {
  struct semaphore *s;
  if ((s = findkey(key)) != 0) {
    release(&s->lock);
    return 1;
  }
  return 0;
}

// Allocate a semaphore in the sems table
// Then, place it in the process' sems table and set the refcount to 1
// Return the index in the process' sems table
// or -1 if the key already exists or if any of the tables is full
int semcreate(int key, int initvalue) {
  struct semaphore *s;
  int semid;

  if (keyexists(key) || (s = semalloc()) == 0) {
    return -1;
  }

  // At this point the sem is locked
  if ((semid = procsemalloc(s)) < 0) {
    release(&s->lock);
    return -1;
  }

  s->key = key;
  s->value = initvalue;
  s->refcount = 1;
  
  release(&s->lock);
  return semid;
}

// Find the semaphore with the given key
// If found, allocate it in the process' sem table
// and increment the refcout
// Return the index in the process' sem table
// or -1 if the key was not found or the process' sem table was full
int semget(int key) {
  struct semaphore *s;
  int semid;

  if ((s = findkey(key)) == 0) {
    return -1;
  }

  // At this point the sem is locked
  if ((semid = procsemalloc(s)) < 0) {
    release(&s->lock);
    return -1;
  }

  s->refcount++;
  release(&s->lock);

  return semid;
}

// Find the semaphore pointer in p->osem[semid] and return it
// Return 0 if semid was out of range
// or no semaphore was in that position
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

  p->osem[semid] = 0;
  acquire(&s->lock);
  if (--(s->refcount) == 0) {
    s->key = -1;
    s->value = 0;
  }
  release(&s->lock);
  
  return 0;
}
