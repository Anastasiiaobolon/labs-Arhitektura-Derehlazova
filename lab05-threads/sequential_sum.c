#include <stdio.h>

#define N 1000000L

int main(void)
{
    long sum = 0;
    for (long i = 1; i <= N; i++) {
        sum += i;
    }
    printf("Sequential result: %ld\n", sum);
    printf("Expected result:   %ld\n", N * (N + 1) / 2);
    return 0;
}
