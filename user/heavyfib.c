#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int fib(int n) {
  char arr[10000];
  arr[0] = 0;
  arr[9999] = arr[0];
  arr[9999] = arr[9999];


  if (n < 1) {
    printf("Out of range\n");
    exit(-1);
  }

  if (n == 1) return 0;
  if (n == 2) return 1;

  return fib(n-1) + fib(n-2);
}

int main(int argc, char *argv[]) {
  int n = atoi(argv[1]);
  printf("%d\n", fib(n));
  return 0;
}