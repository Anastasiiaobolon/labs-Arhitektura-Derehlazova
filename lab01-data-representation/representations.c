#include <stdio.h>

static void print_bits(unsigned int value)
{
    /* Знаходимо старший біт, що дорівнює 1, щоб не друкувати зайві нулі */
    int top = 31;
    while (top > 0 && ((value >> top) & 1u) == 0) {
        top--;
    }
    for (int i = top; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    unsigned int values[] = {10, 65, 127, 255, 256, 1024};
    size_t count = sizeof(values) / sizeof(values[0]);

    printf("%-10s %-12s %s\n", "Decimal", "Hexadecimal", "Binary");
    for (size_t i = 0; i < count; i++) {
        printf("%-10u 0x%-10X ", values[i], values[i]);
        print_bits(values[i]);
        printf("\n");
    }
    return 0;
}
