#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint32_t value = 0x12345678;
    unsigned char *p = (unsigned char *)&value;

    printf("value = 0x%08X\n\n", value);
    printf("%-20s %s\n", "Address", "Value");
    for (int i = 0; i < 4; i++) {
        printf("%-20p 0x%02X\n", (void *)&p[i], p[i]);
    }

    /* Визначаємо порядок байтів: що лежить за найменшою адресою? */
    if (p[0] == 0x78) {
        printf("\nByte order: Little-endian\n");
    } else if (p[0] == 0x12) {
        printf("\nByte order: Big-endian\n");
    } else {
        printf("\nByte order: unknown\n");
    }
    return 0;
}
