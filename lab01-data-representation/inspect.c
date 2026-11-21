#include <stdio.h>
#include <stdint.h>

static void print_binary(uint8_t value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

/* Повертає 1 для little-endian, 0 для big-endian (визначається експериментально) */
static int is_little_endian(void)
{
    uint32_t x = 0x12345678;
    unsigned char *p = (unsigned char *)&x;
    return p[0] == 0x78;
}

static void inspect_byte(void)
{
    unsigned int input;

    printf("Enter value (0-255): ");
    if (scanf("%u", &input) != 1 || input > 255) {
        fprintf(stderr, "Invalid value: expected an integer in range 0-255\n");
        return;
    }

    uint8_t value = (uint8_t)input;
    printf("Decimal:      %u\n", value);
    printf("Hexadecimal:  0x%02X\n", value);
    printf("Binary:       ");
    print_binary(value);
    printf("\n");
    if (value >= 32 && value <= 126) {
        printf("Character:    %c\n", value);
    } else {
        printf("Character:    (not printable)\n");
    }
}

static void inspect_word(void)
{
    uint32_t value = 0x12345678;
    unsigned char *p = (unsigned char *)&value;

    printf("\nValue: 0x%08X\n", value);
    printf("Size: %zu bytes\n", sizeof(value));
    printf("Memory:\n");
    for (size_t i = 0; i < sizeof(value); i++) {
        printf("byte %zu: 0x%02X\n", i, p[i]);
    }
    printf("Byte order: %s\n", is_little_endian() ? "Little-endian" : "Big-endian");
}

int main(void)
{
    inspect_byte();
    inspect_word();
    return 0;
}
