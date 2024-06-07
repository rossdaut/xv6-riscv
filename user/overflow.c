#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void recursive_function(int count) {
    printf("Recursion depth: %d\n", count);
    sleep(1);
    if (count < 1000000) {   // Higher values will probably cause a stack overflow
        recursive_function(count + 1);
    }
}

int main(int argc, char *argv[]) {
    printf("Starting stack overflow test\n");
    recursive_function(1);
    exit(0);
}
