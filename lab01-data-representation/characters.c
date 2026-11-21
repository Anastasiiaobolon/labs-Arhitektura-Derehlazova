#include <stdio.h>

int main(void)
{
    char chars[] = {'A', 'B', 'a', '0', '9'};
    size_t count = sizeof(chars) / sizeof(chars[0]);

    printf("%-10s %-8s %s\n", "Character", "Decimal", "Hex");
    for (size_t i = 0; i < count; i++) {
        printf("%-10c %-8d 0x%X\n", chars[i], chars[i], (unsigned char)chars[i]);
    }
    return 0;
}
