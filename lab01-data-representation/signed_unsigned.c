#include <stdio.h>
#include <stdint.h>

static void print_binary(uint8_t value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    int8_t signed_value = -1;
    uint8_t unsigned_value = 255;

    printf("int8_t  signed_value   = %d\n", signed_value);
    printf("uint8_t unsigned_value = %u\n", unsigned_value);

    printf("signed_value   hex: 0x%02X, binary: ", (uint8_t)signed_value);
    print_binary((uint8_t)signed_value);
    printf("\n");

    printf("unsigned_value hex: 0x%02X, binary: ", unsigned_value);
    print_binary(unsigned_value);
    printf("\n");

    printf("Same bits? %s\n",
           ((uint8_t)signed_value == unsigned_value) ? "yes" : "no");
    return 0;
}
