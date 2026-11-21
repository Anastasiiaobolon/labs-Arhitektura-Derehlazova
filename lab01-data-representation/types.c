#include <stdio.h>

int main(void)
{
    printf("%-12s %-8s %s\n", "Type", "Bytes", "Bits");
    printf("%-12s %-8zu %zu\n", "char", sizeof(char), sizeof(char) * 8);
    printf("%-12s %-8zu %zu\n", "short", sizeof(short), sizeof(short) * 8);
    printf("%-12s %-8zu %zu\n", "int", sizeof(int), sizeof(int) * 8);
    printf("%-12s %-8zu %zu\n", "long", sizeof(long), sizeof(long) * 8);
    printf("%-12s %-8zu %zu\n", "long long", sizeof(long long), sizeof(long long) * 8);
    printf("%-12s %-8zu %zu\n", "float", sizeof(float), sizeof(float) * 8);
    printf("%-12s %-8zu %zu\n", "double", sizeof(double), sizeof(double) * 8);
    printf("%-12s %-8zu %zu\n", "void *", sizeof(void *), sizeof(void *) * 8);
    return 0;
}
