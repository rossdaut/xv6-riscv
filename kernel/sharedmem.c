#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "proc.h"

#define PROCVA(p, shmid) (p->brk + shmid * MAXSHMSIZE * PGSIZE)
#define SIZE2PG(size) ((size + PGSIZE - 1) / PGSIZE)

struct sharedmem {
  int key;
  int size;
  int refcount;
  uint64 phyaddr[MAXSHMSIZE];
  struct spinlock lock;
};

struct sharedmem sharedmems[NSHM];

// Auxiliary functions
int proc_find_free_shmid();
struct sharedmem *find_by_key(int key);
struct sharedmem *find_free_descriptor();
struct sharedmem *alloc_descriptor(int key, int size);
void dealloc_descriptor(struct sharedmem *shm);

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
    for (int i = 0; i < MAXSHMSIZE; i++)
      shm->phyaddr[i] = 0;
  }
}

// Allocate a shared block in the sharedmems table if the key does not exist
// ...
int shmget(int key, int size, void **addr) {
  struct proc *p = myproc();
  struct sharedmem *shm;
  int shmid, i, mappedpages;
  int pages = SIZE2PG(size);
  uint64 va, ith_va;

  if (size < 1 || pages > MAXSHMSIZE) {
    printf("Kernel shmget: shm block invalid size");
    return -1;
  }

  // Look for free shmid in process
  shmid = proc_find_free_shmid();
  if (shmid == -1) {
    printf("Kernel shmget: no shm blocks available in process\n");
    return -1;
  }

  // Check if exists a shm block with given key
  if ((shm = find_by_key(key)) == 0) {
    // No shm with given key exists
    if ((shm = alloc_descriptor(key, size)) == 0) {
      printf("Kernel shmget: could not alloc a shm descriptor\n");
      return -1;
    }
  }
  // Either find_by_key() or alloc_proc() give us a locked shm descriptor.

  va = PROCVA(p, shmid);
  mappedpages = 0;
  for (i = 0; i < pages; i++) {
    ith_va = va + i * PGSIZE;
    if (mappages(p->pagetable, ith_va, PGSIZE, (uint64)shm->phyaddr[i], PTE_W | PTE_R | PTE_U) != 0) {
      printf("Kernel shmget: could not map the %d-th page\n", i);
      goto bad;
    }
    mappedpages++;
  }
  
  shm->refcount++;
  p->oshm[shmid].shm = shm;
  p->oshm[shmid].va = va;
  release(&shm->lock);

  copyout(p->pagetable, (uint64) addr, (char *) &va, sizeof(uint64));
  return shmid;

bad:
  if(shm->refcount == 0) {
    //if shm was allocated by this process
    dealloc_descriptor(shm);
  }
  uvmunmap(p->pagetable, va, mappedpages, 0);
  return -1;
}

int shmclose(int shmid) {
  struct proc *p = myproc();
  struct sharedmem *shm;

  if (shmid < 0 || shmid >= NSHMPROC) {
    printf("Kernel shmclose: invalid shmid\n");
    return -1;
  }
  
  shm = p->oshm[shmid].shm;

  if (shm == 0) {
    return -1;
  }
  
  uvmunmap(p->pagetable, p->oshm[shmid].va, SIZE2PG(shm->size), 0);
  p->oshm[shmid].shm = 0;
  p->oshm[shmid].va = 0;
    
  acquire(&shm->lock);
  if (--shm->refcount == 0){
    dealloc_descriptor(shm);
  }
  release(&shm->lock);

  return 0;
}

struct sharedmem *find_by_key(int key) {
  struct sharedmem *shm;

  for (shm = sharedmems; shm < &sharedmems[NSHM]; shm++) {
    acquire(&shm->lock);
    if (shm->key == key) {
      return shm;
    }
    release(&shm->lock);
  }

  return 0;
}

int proc_find_free_shmid() {
  struct proc *p = myproc();
  int shmid;

  for (shmid = 0; shmid < NSHMPROC; shmid++) {
    if (p->oshm[shmid].shm == 0) {
      return shmid;
    }
  }
  
  return -1;
}

struct sharedmem *find_free_descriptor() {
  struct sharedmem *shm;

  for (shm = sharedmems; shm < &sharedmems[NSHM]; shm++) {
    acquire(&shm->lock);
    if (shm->refcount == 0) {
      return shm;
    }
    release(&shm->lock);
  }

  return 0;
}

struct sharedmem *alloc_descriptor(int key, int size) {
  struct sharedmem *shm;
  int pages = SIZE2PG(size);

  shm = find_free_descriptor();
  if (shm == 0) {
    printf("alloc_descriptor: no free descriptors\n");
    return 0;
  }
  
  shm->key = key;
  shm->size = size;

  for (int i = 0; i < pages; i++) {
    shm->phyaddr[i] = (uint64) kalloc();
    if (shm->phyaddr[i] == 0) {
      printf("Kernel alloc_descriptor: could not alloc a physical page\n");
      dealloc_descriptor(shm);
      return 0;
    }
  }

  return shm;
}

void dealloc_descriptor(struct sharedmem *shm) {
  shm->key = -1;
  shm->size = 0;
  shm->refcount = 0;

  for (int i = 0; i < MAXSHMSIZE; i++) {
    if (shm->phyaddr[i] == 0)
      continue;

    kfree((void *) shm->phyaddr[i]);
    shm->phyaddr[i] = 0;
  }
}
