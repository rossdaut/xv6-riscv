#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/param.h"
#include "user/user.h"

#define NELEM(arr) (sizeof(arr)/sizeof(arr[0]))

//Test functions
int fulloshm(void);
int writeread(void);

struct test {
    int (*f)(void);
    char *name;
} tests[] = {
    {fulloshm, "fulloshm"},
    {writeread, "writeread"},
};

int main()
{
    struct test *test;
    for (test = tests; test < &tests[NELEM(tests)]; test++) {
        printf("\nRunning %s test:\n", test->name);
        if(test->f() == -1) {
            return -1;
        }
    }

    return 0;
}

int
fulloshm(void)
{
    int i;
    char *str;
    int shmids[NSHMPROC];

    for(i = 0; i < NSHMPROC; i++) {
        shmids[i] = shmget(i, MAXSHMSIZE, (void*)&str);
    }
    if(shmget(i, MAXSHMSIZE, (void*)&str) == -1) {
        printf("-> fulloshm test PASSED: shmget failed since oshem is full\n");
    } else {
        printf("-> fulloshm test FAILED: shmget success when oshem is full\n");
        return -1;
    }
    for(i = 0; i < NSHMPROC; i++) {
        shmclose(shmids[i]);
    }
    return 0;
}

int
writeread(void)
{
    int i, child_status;
    char *str;
    int shmid;

    shmid = shmget(15, MAXSHMSIZE * PGSIZE, (void*)&str);
    
    for(i = 0; i < MAXSHMSIZE * PGSIZE; i++) {
        str[i] = 'a';
    }

    if (fork()==0) {
        shmid = shmget(15, MAXSHMSIZE * PGSIZE, (void*)&str);
        for(i = 0; i < MAXSHMSIZE * PGSIZE; i++) {
            if (str[i] != 'a') {
                printf("-> writeread test FAILED: child process failed to read the %d-th byte from shared memory\n", i);
                exit(-1);
            }
        }
        printf("-> writeread test PASSED: child process read from shared memory\n");

        shmclose(shmid);
        exit(0);
    }

    wait(&child_status);

    shmclose(shmid);
    return child_status;
}
