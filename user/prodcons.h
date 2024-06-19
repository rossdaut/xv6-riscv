#define NPRODS 1
#define NCONS 1
#define NTASK 1
#define BUFSIZE 10

#define BUFKEY 15
#define BUFNAME "buffer"

#define FULLKEY 0
#define EMPTYKEY 1
#define MUTEXKEY 2

struct buffer {
    int data[BUFSIZE];
    int head;
    int tail;
    int size;
};