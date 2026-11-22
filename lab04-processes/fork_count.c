#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/* Експеримент: скільки разів виконається printf після двох fork() */
int main(void)
{
    fork();
    fork();
    printf("PID=%d PPID=%d\n", getpid(), getppid());
    fflush(stdout);
    while (wait(NULL) > 0) {
        /* чекаємо на всіх своїх дітей */
    }
    return 0;
}
