#include <stdio.h>
#include <unistd.h>

/*
 * Звичайна збірка:    gcc -Wall -Wextra buffering.c -o buffering
 * З fflush():         gcc -Wall -Wextra -DUSE_FFLUSH buffering.c -o buffering_flush
 */

int main(void)
{
    printf("A");
    printf("B");
    printf("C");

#ifdef USE_FFLUSH
    fflush(stdout);
#endif

    write(STDOUT_FILENO, "D", 1);
    return 0;
}
