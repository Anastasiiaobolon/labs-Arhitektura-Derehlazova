#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Before exec: PID=%d\n", getpid());
    fflush(stdout);

    execlp("echo", "echo", "Hello from exec!", NULL);

    /* Сюди ми потрапимо лише якщо exec не вдався */
    perror("execlp");
    return EXIT_FAILURE;
}
