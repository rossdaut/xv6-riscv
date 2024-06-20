#define NPRODS 3
#define NCONS 3
#define NTASK 4
#define BUFSIZE 7

#define BUFKEY 15
#define BUFNAME "buffer"

#define FULLKEY 0
#define EMPTYKEY 1
#define MUTEXKEY 2

struct buffer {
    int data[BUFSIZE][2];
    int head;
    int tail;
    int size;
};