#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void recursive_function(int count) {
    // Usamos el buffer y lo llenamos con valores
    char buffer[1024];
    int sum = 0;
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = 'A';
        sum += buffer[i];
    }

    printf("Recursion depth: %d, sum: %d\n", count, sum);
    sleep(1); // Añade un pequeño delay para observar la salida
    if (count < 1000000) { // Límite alto para evitar advertencia de recursión infinita
        recursive_function(count + 1); // Llama recursivamente a sí mismo
    }
}

int main(int argc, char *argv[]) {
    printf("Starting stack overflow test\n");
    recursive_function(1); // Inicia la recursión
    exit(0);
}
