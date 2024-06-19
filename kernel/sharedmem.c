#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "proc.h"

#define PROCVA(p, shmid) (p->shmbase + shmid * PGSIZE)

struct sharedmem {
  int key;
  int size;
  int refcount;
  uint64 phyaddr;
  struct spinlock lock;
};

struct sharedmem sharedmems[NSHM];

// Auxiliary functions
struct sharedmem *find_by_key(int key);
struct sharedmem *find_free_descriptor();
int shared_proc_alloc(struct sharedmem *shm);

// Initialize the shared memory table
// For each one, initialize its lock and variables
// This function should be run at system init (main.c)
void shminit() {
  struct sharedmem *shm;

  for(shm = sharedmems; shm < &sharedmems[NSHM]; shm++) {
    initlock(&shm->lock, "shm");
    shm->key = -1;
    shm->size = 0;
    shm->refcount = 0;
    shm->phyaddr = 0;
  }
}

// Allocate a shared block in the sharedmems table if the key does not exist
// ...
int shmget(int key, int size, void **addr) {
  struct proc *p = myproc();
  struct sharedmem *shm;
  int shmid;
  int created = 0;
  uint64 va;

  shm = find_by_key(key);
  if (shm == 0) {
    // Key not found, allocate a new block
    if ((shm = find_free_descriptor()) == 0) {
      return -1;
    }
    created = 1;
  }

  if ((shmid = shared_proc_alloc(shm)) == -1) {
    release(&shm->lock);
    return -1;
  }

  if (created) {
    shm->phyaddr = (uint64)kalloc();
  }

  va = PROCVA(p, shmid);

  // Map the shared block in the process' page table
  if(mappages(p->pagetable, va, size, shm->phyaddr, PTE_R | PTE_W | PTE_U) != 0) {
    release(&shm->lock);
    return -1;
  }

  if (created) {
    shm->key = key;
    shm->size = size;
  }
  shm->refcount++;
  release(&shm->lock);

  // *addr = (void *) va;
  copyout(p->pagetable, (uint64)addr, (char*)&va, 8);
  return shmid;
}

// Free a shared block in the sharedmems table
// ...


// Find a shared block with the given key in sharedmems table
// If found, return shared block pointer with its lock held.
// Otherwise, return 0.
struct sharedmem *find_by_key(int key) {
    struct sharedmem *shm;

    for(shm = sharedmems; shm < &sharedmems[NSHM]; shm++) {
        acquire(&shm->lock);
        if(shm->key == key) {
            return shm;
        }
        release(&shm->lock);
    }

    return 0;
}

// Find a free shared block in the sharedmems table
// If found, return shared block pointer with its lock held.
// Otherwise, return 0.
struct sharedmem *find_free_descriptor() {
    struct sharedmem *shm;

    for(shm = sharedmems; shm < &sharedmems[NSHM]; shm++) {
        acquire(&shm->lock);
        if(shm->refcount == 0) {
            return shm;
        }
        release(&shm->lock);
    }

    return 0;
}

// Allocate a semaphore pointer in the process' shared blocks table 
// Return the index in the table
// Return -1 if the table is full
int shared_proc_alloc(struct sharedmem *shm) {
    struct proc *p = myproc();
    int shmid;

    for(shmid = 0; shmid < NSHMPROC; shmid++) {
        if(p->oshm[shmid] == 0) {
            p->oshm[shmid] = shm;
            return shmid;
        }
    }

    return -1;
}