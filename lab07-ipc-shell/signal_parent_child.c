#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Звичайна збірка (default поведінка SIGTERM):
 *     gcc -Wall -Wextra signal_parent_child.c -o signal_parent_child
 * Збірка з власним handler для SIGTERM:
 *     gcc -Wall -Wextra -DCUSTOM_HANDLER signal_parent_child.c -o signal_parent_child_handler
 */

#ifdef CUSTOM_HANDLER
static void handle_sigterm(int signal_number)
{
    (void)signal_number;
    const char message[] = "Child: SIGTERM received, exiting gracefully\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
    _exit(0);
}
#endif

int main(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
#ifdef CUSTOM_HANDLER
        struct sigaction action = {0};
        action.sa_handler = handle_sigterm;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGTERM, &action, NULL) == -1) {
            perror("sigaction");
            _exit(EXIT_FAILURE);
        }
#endif
        printf("Child: PID=%d, waiting for a signal...\n", getpid());
        fflush(stdout);
        while (1) {
            pause();
        }
    }

    sleep(2);
    printf("Parent: sending SIGTERM to child %d\n", pid);
    fflush(stdout);
    if (kill(pid, SIGTERM) == -1) {
        perror("kill");
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("Parent: child terminated by signal %d\n", WTERMSIG(status));
    } else if (WIFEXITED(status)) {
        printf("Parent: child exited with status %d\n", WEXITSTATUS(status));
    }
    return EXIT_SUCCESS;
}
