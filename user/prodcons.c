#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "prodcons.h"

int bufferd;
char numstr[5] = "0000";

char inc(char c) {
  c++;
  if (c == '9' + 1) {
    c = '0';
  }
  return c;
}

char dec(char c) {
  c--;
  if (c == '0' - 1) {
    c = '9';
  }
  return c;
}

void increment() {
  for (int i = 3; i >= 0; i--) {
    numstr[i] = inc(numstr[i]);
    if (numstr[i] != '0') break;
  }
}

void decrement() {
  for (int i = 3; i >= 0; i--) {
    numstr[i] = dec(numstr[i]);
    if (numstr[i] != '9') break;
  }
}

int main(int argc, char *argv[]) {
  // Initialize the buffer file
  bufferd = open(BUFNAME, O_CREATE|O_WRONLY|O_TRUNC);
  write(bufferd, "0000\n", 5);
  close(bufferd);

  int full_semid = semcreate(FULLKEY, BUFSIZE);
  int empty_semid = semcreate(EMPTYKEY, 0);
  int mutex_semid = semcreate(MUTEXKEY, 1);
  int i;

  for (i = 0; i < NPRODS; i++) {
    if (fork() == 0) {
      full_semid = semget(FULLKEY);
      empty_semid = semget(EMPTYKEY);
      mutex_semid = semget(MUTEXKEY);

      semwait(full_semid);
      semwait(mutex_semid);

      bufferd = open(BUFNAME, O_RDONLY);
      read(bufferd, numstr, 4);
      increment();
      close(bufferd);
      bufferd = open(BUFNAME, O_WRONLY|O_TRUNC);
      write(bufferd, numstr, 4);
      printf("prod %d: %s\n", getpid(), numstr);

      semsignal(mutex_semid);
      semsignal(empty_semid);

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

      bufferd = open(BUFNAME, O_RDONLY);
      read(bufferd, numstr, 4);
      decrement();
      close(bufferd);
      bufferd = open(BUFNAME, O_WRONLY|O_TRUNC);
      write(bufferd, numstr, 4);
      printf("cons %d: %s\n", getpid(), numstr);

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
