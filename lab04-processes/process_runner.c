#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Parent PID: %d\n", getpid());
    printf("Creating child...\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("Child PID: %d\n", getpid());
        printf("Child PPID: %d\n", getppid());
        printf("Executing: %s\n", argv[1]);
        fflush(stdout);

        execlp(argv[1], argv[1], (char *)NULL);

        /* exec повернувся, значить сталася помилка */
        fprintf(stderr, "exec %s failed: %s\n", argv[1], strerror(errno));
        _exit(127);
    }

    printf("Child created, PID: %d\n", pid);
    fflush(stdout);

    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("Child %d completed\n", pid);
        printf("Exit status: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child %d terminated by signal %d\n", pid, WTERMSIG(status));
    } else {
        printf("Child %d ended abnormally\n", pid);
    }

    printf("Parent continues\n");
    return EXIT_SUCCESS;
}
