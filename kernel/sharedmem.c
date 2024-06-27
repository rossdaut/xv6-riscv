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

/* 
 * Initialize the shared memory table
 * For each one, initialize its lock and variables
 * This function should be run at system init (main.c)
*/
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

/*
 * Allocate a shared block in the sharedmems table if the key does not exist.
 * Then, map the block's physical pages to process' virtual addresses.
 * Place it in the process' oshm table if there is a shmid available.
 * Return the shmid on success. 
 * Valid key values are non-negative integers.
*/
int shmget(int key, int size, void **addr) {
  struct proc *p = myproc();
  struct sharedmem *shm;
  int shmid, i, mappedpages;
  int pages = SIZE2PG(size);
  uint64 va, ith_va;

  if (key < 0 || size < 1 || pages > MAXSHMSIZE) {
    printf("Kernel shmget: shm invalid args");
    return -1;
  }

  // Look for a free shmid in process
  shmid = proc_find_free_shmid();
  if (shmid == -1) {
    printf("Kernel shmget: no shm blocks available in process\n");
    return -1;
  }

  // Check if exists a shm block with given key
  if ((shm = find_by_key(key)) == 0) {
    // If not, allocate a new shm block
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

/* 
 * Unmap block's physical pages in process pagetable and update its oshem table.
 * Decrement shm refcount and dealloc descriptor if it reaches 0.
 * Return 0 on success.
 * Return -1 if shmid is invalid or there is no shm open at shmid.
*/
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

/*
 * Find shm descriptor with the given key in sharedmems table
 * If found, return shm pointer with its lock held.
 * Otherwise, return 0.
*/
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

/*
 * Find free shmid in process' oshem table
 * If found, return shmid. Otherwise, return -1.
*/
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

/*
 * Look for a free shm descriptor in the sharedmems table.
 * That is, a descriptor with refcount == 0.
 * Return a pointer to a locked descriptor.
 * Return 0 if there are no free descriptors.
*/
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

/*
 * Find a free shm descriptor in the sharedmems table.
 * Initialize its fields and allocate as many physical pages
 * as needed by the size argument.
 * Return a pointer to the descriptor.
 * Return 0 if there are no free descriptors or a physical page
 * could not be allocated.
*/
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

/*
 * Deallocate a shared memory descriptor.
 * This function should be called when the refcount of a shared memory descriptor reaches 0.
 * It frees the physical pages associated with the descriptor
 * and resets the descriptor's fields
*/
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
