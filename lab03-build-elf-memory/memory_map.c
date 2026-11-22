#include <stdio.h>
#include <stdlib.h>

int global_initialized = 100;
int global_uninitialized;
static int static_initialized = 200;
static int static_uninitialized;
const char *string_literal = "Computer Systems";

void show_function(void)
{
}

int main(void)
{
    int local = 300;
    int *heap_value = malloc(sizeof(int));
    if (heap_value == NULL) {
        return 1;
    }
    *heap_value = 400;

    printf("Code/function:           %p\n", (void *)show_function);
    printf("Global initialized:      %p\n", (void *)&global_initialized);
    printf("Global uninitialized:    %p\n", (void *)&global_uninitialized);
    printf("Static initialized:      %p\n", (void *)&static_initialized);
    printf("Static uninitialized:    %p\n", (void *)&static_uninitialized);
    printf("String literal:          %p\n", (const void *)string_literal);
    printf("Heap allocation:         %p\n", (void *)heap_value);
    printf("Stack/local:             %p\n", (void *)&local);

    free(heap_value);
    return 0;
}
