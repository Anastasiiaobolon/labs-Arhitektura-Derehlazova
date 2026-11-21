#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint8_t value = 255;

    printf("Before: value = %u\n", value);
    value++;
    printf("After value++: value = %u\n", value);

    printf("\nCheck:\n");
    uint8_t a = 254;
    uint8_t b = 255;
    uint8_t c = (uint8_t)(255 + 1);
    uint8_t d = (uint8_t)(255 + 2);
    printf("254     -> %u\n", a);
    printf("255     -> %u\n", b);
    printf("255 + 1 -> %u\n", c);
    printf("255 + 2 -> %u\n", d);
    return 0;
}
