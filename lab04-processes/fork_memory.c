#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int value = 100;

int main(void)
{
    printf("Before fork: value=%d, address=%p\n", value, (void *)&value);
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        value = 200;
        printf("CHILD:  PID=%d value=%d address=%p\n",
               getpid(), value, (void *)&value);
    } else {
        value = 300;
        printf("PARENT: PID=%d value=%d address=%p\n",
               getpid(), value, (void *)&value);
        wait(NULL); /* щоб shell не повернув prompt раніше за child */
    }

    return EXIT_SUCCESS;
}
