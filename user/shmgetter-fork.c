#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    char *str;
    char hola[] = "hola, mundo!";

    shmget(0, 4096, (void*)&str);
    printf("shmgetter: %p\n", str);
    for (int i = 0; i < sizeof(hola); i++)
    {
        str[i] = hola[i];
    }

    if (fork() == 0) {
        printf("child: %s\n", str);
        return 0;
    }

    wait(0);

    return 0;
}
