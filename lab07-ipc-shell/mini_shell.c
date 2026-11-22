#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINE_SIZE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 2   /* підтримується одна pipe: cmd1 | cmd2 */

/* Одна розібрана команда: аргументи + необов'язкове перенаправлення виводу */
typedef struct {
    char *argv[MAX_ARGS];
    char *outfile;       /* файл для ">" або NULL */
} Command;

/* ---------- Parsing ---------- */

/*
 * Розбирає рядок на команди, розділені "|".
 * Повертає кількість команд (0 для порожнього рядка, -1 для синтаксичної помилки).
 * Parser навмисно простий: токени розділені пробілами, лапки не підтримуються.
 */
static int parse_line(char *line, Command commands[])
{
    int count = 0;
    int argc = 0;
    int expect_outfile = 0;

    memset(&commands[0], 0, sizeof(Command));

    char *saveptr = NULL;
    for (char *token = strtok_r(line, " \t\r\n", &saveptr);
         token != NULL;
         token = strtok_r(NULL, " \t\r\n", &saveptr)) {

        if (strcmp(token, "|") == 0) {
            if (argc == 0 || expect_outfile) {
                fprintf(stderr, "myshell: syntax error near '|'\n");
                return -1;
            }
            if (count + 1 >= MAX_COMMANDS) {
                fprintf(stderr, "myshell: only one pipe is supported\n");
                return -1;
            }
            commands[count].argv[argc] = NULL;
            count++;
            argc = 0;
            memset(&commands[count], 0, sizeof(Command));
        } else if (strcmp(token, ">") == 0) {
            if (argc == 0 || expect_outfile) {
                fprintf(stderr, "myshell: syntax error near '>'\n");
                return -1;
            }
            expect_outfile = 1;
        } else if (expect_outfile) {
            commands[count].outfile = token;
            expect_outfile = 0;
        } else {
            if (argc >= MAX_ARGS - 1) {
                fprintf(stderr, "myshell: too many arguments\n");
                return -1;
            }
            commands[count].argv[argc++] = token;
        }
    }

    if (expect_outfile) {
        fprintf(stderr, "myshell: expected file name after '>'\n");
        return -1;
    }
    if (argc == 0) {
        return count == 0 ? 0 : (fprintf(stderr, "myshell: syntax error: missing command after '|'\n"), -1);
    }

    commands[count].argv[argc] = NULL;
    return count + 1;
}

/* ---------- Child setup ---------- */

/* Виконується в child ПЕРЕД exec: відновлення сигналів і перенаправлення */
static void setup_child(const Command *cmd)
{
    /* Shell ігнорує SIGINT, а ігнорування успадковується через exec,
       тому child повертає default поведінку. */
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        _exit(EXIT_FAILURE);
    }

    if (cmd->outfile != NULL) {
        int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            fprintf(stderr, "myshell: %s: %s\n", cmd->outfile, strerror(errno));
            _exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            _exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

static void exec_command(const Command *cmd)
{
    execvp(cmd->argv[0], cmd->argv);
    fprintf(stderr, "myshell: %s: %s\n", cmd->argv[0], strerror(errno));
    _exit(127);
}

static void wait_for(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }
        perror("waitpid");
        return;
    }
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
        write(STDOUT_FILENO, "\n", 1);       /* новий рядок після Ctrl+C */
    }
}

/* ---------- Execution ---------- */

static void run_single(const Command *cmd)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        setup_child(cmd);
        exec_command(cmd);
    }
    wait_for(pid);
}

static void run_pipeline(const Command *left, const Command *right)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (pid1 == 0) {
        /* Child 1: stdout -> pipe */
        setup_child(left);
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        exec_command(left);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        wait_for(pid1);
        return;
    }
    if (pid2 == 0) {
        /* Child 2: stdin <- pipe (stdout може бути перенаправлений через ">") */
        setup_child(right);
        close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        exec_command(right);
    }

    /* Parent закриває обидва кінці pipe, інакше child 2 не побачить EOF */
    close(pipefd[0]);
    close(pipefd[1]);
    wait_for(pid1);
    wait_for(pid2);
}

/* ---------- Built-in commands ---------- */

/* Повертає 1, якщо команда була built-in і вже виконана */
static int run_builtin(const Command *cmd, int *should_exit)
{
    if (strcmp(cmd->argv[0], "exit") == 0) {
        *should_exit = 1;
        return 1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *target = cmd->argv[1] != NULL ? cmd->argv[1] : getenv("HOME");
        if (target == NULL) {
            fprintf(stderr, "myshell: cd: no directory specified\n");
        } else if (chdir(target) == -1) {
            fprintf(stderr, "myshell: cd: %s: %s\n", target, strerror(errno));
        }
        return 1;
    }

    return 0;
}

/* ---------- main loop ---------- */

int main(void)
{
    /* Shell ігнорує Ctrl+C: завершується лише foreground-команда */
    struct sigaction ignore;
    memset(&ignore, 0, sizeof(ignore));
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    if (sigaction(SIGINT, &ignore, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    char line[LINE_SIZE];
    Command commands[MAX_COMMANDS];
    int should_exit = 0;

    while (!should_exit) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            printf("\n");          /* Ctrl+D (EOF) */
            break;
        }

        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            fprintf(stderr, "myshell: line too long\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* пропускаємо залишок довгого рядка */
            }
            continue;
        }

        int count = parse_line(line, commands);
        if (count <= 0) {
            continue;              /* порожній рядок або помилка парсера */
        }

        if (count == 1) {
            if (run_builtin(&commands[0], &should_exit)) {
                continue;
            }
            run_single(&commands[0]);
        } else {
            run_pipeline(&commands[0], &commands[1]);
        }
    }

    return EXIT_SUCCESS;
}
