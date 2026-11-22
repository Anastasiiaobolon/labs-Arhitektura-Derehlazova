#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Експеримент: child читає pipe до EOF.
 *
 *   gcc -Wall -Wextra pipe_no_close.c -o pipe_no_close            -> parent НЕ закриває write end у child (зависає)
 *   gcc -Wall -Wextra -DCLOSE_UNUSED pipe_no_close.c -o pipe_ok   -> всі зайві descriptors закриті (EOF працює)
 *
 * Запускайте з обмеженням часу:  timeout 5 ./pipe_no_close
 */

int main(void)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child: читає */
#ifdef CLOSE_UNUSED
        close(pipefd[1]);          /* child не пише, закриваємо write end */
#endif
        char buffer[128];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            printf("Child received: %s", buffer);
            fflush(stdout);
        }
        printf("Child: EOF reached (read returned %zd)\n", n);
        fflush(stdout);               /* _exit() не скидає буфер stdio */
        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    }

    /* Parent: пише і закриває свій write end, але... */
    close(pipefd[0]);
    const char message[] = "Message from parent\n";
    write(pipefd[1], message, strlen(message));
    close(pipefd[1]);

    waitpid(pid, NULL, 0);
    return EXIT_SUCCESS;
}
