#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define NPRODS 5
#define NCONS 5
#define BUFSIZE 10

#define FULLKEY 0
#define EMPTYKEY 1
#define MUTEXKEY 2

struct {
    char array[BUFSIZE];
    int head;
    int tail;
    int size;
} *buffer;

void bufinit() {
    buffer = malloc(sizeof(*buffer));
    buffer->head = 0;
    buffer->tail = 0;
    buffer->size = 0;
}

void enqueue(char c) {
    buffer->array[buffer->tail] = c;
    printf("enqueue: %c\n", c);
    buffer->tail = (buffer->tail + 1) % BUFSIZE;
    buffer->size++;
}

char dequeue() {
    char c = buffer->array[buffer->head];
    printf("dequeue: %c\n", c);
    buffer->head = (buffer->head + 1) % BUFSIZE;
    buffer->size--;
    return c;
}

int main(int argc, char *argv[]) {
    int full_semid = semcreate(FULLKEY, BUFSIZE);
    int empty_semid = semcreate(EMPTYKEY, 0);
    int mutex_semid = semcreate(MUTEXKEY, 1);
    int i;

    bufinit();
    
    for (i = 0; i < NPRODS; i++) {
        if (fork() == 0) {
            full_semid = semget(FULLKEY);
            empty_semid = semget(EMPTYKEY);
            mutex_semid = semget(MUTEXKEY);
            
            semwait(full_semid);
            semwait(mutex_semid);
            
            char c = 'a';
            enqueue(c);
            //printf("prod %d: %c\n", getpid(), c);

            semsignal(mutex_semid);
            semsignal(empty_semid);

            semclose(full_semid);
            semclose(empty_semid);
            semclose(mutex_semid);
            return 0;
        }
    }

    for (i = 0; i < NCONS; i++) {
        if (fork() == 0) {
            full_semid = semget(FULLKEY);
            empty_semid = semget(EMPTYKEY);
            mutex_semid = semget(MUTEXKEY);

            semwait(empty_semid);
            semwait(mutex_semid);

            //char c = 
            dequeue();
            //printf("cons %d: %c\n", getpid(), c);
            
            semsignal(mutex_semid);
            semsignal(full_semid);

            semclose(full_semid);
            semclose(empty_semid);
            semclose(mutex_semid);
            return 0;
        }
    }

    for (i = 0; i < NPRODS + NCONS; i++) {
        wait(0);
    }

    semclose(full_semid);
    semclose(empty_semid);
    semclose(mutex_semid);
    return 0;
}
