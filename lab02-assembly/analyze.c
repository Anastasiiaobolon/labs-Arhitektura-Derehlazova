#include <stdio.h>

/*
 * Порівнює два числа, потім у циклі повторює обчислення.
 * Містить: функцію, два параметри, арифметику, if/else, цикл, return.
 */
int analyze(int a, int b)
{
    int result = 0;

    if (a > b) {
        result = a - b;
    } else {
        result = a + b;
    }

    for (int i = 0; i < 3; i++) {
        result = result * 2;
    }

    return result;
}

int main(void)
{
    printf("%d\n", analyze(10, 4));
    printf("%d\n", analyze(3, 8));
    return 0;
}
