#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "interpreter.h"

int main()
{
    // Input buffer for user input, can hold 49 chars + \0
    size_t length = 0;
    char input_buffer[50] = {0};
    int c = 0;
    while (1)
    {
        printf(">>>");
        if (fgets(input_buffer, sizeof(input_buffer), stdin))
        {
            length = strlen(input_buffer);
            if (strchr(input_buffer, '\n') == NULL)
            {
                // Ignore the line with too much input characters
                while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }
        }
        printf("%d\n", expr(input_buffer, length));
    }
    return 0;
}

