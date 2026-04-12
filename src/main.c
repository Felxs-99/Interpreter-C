#include "main.h"
#include "interpreter.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    Interpreter interpret = {0};
    init_interpreter(
        &interpret); // (Assuming you split this like we discussed!)

    while (1)
    {
        // 1. readline handles the prompt AND reads the keystrokes!
        char *input_buffer = readline(">>> ");

        // 2. If the user presses Ctrl+D (EOF), input_buffer is NULL
        if (input_buffer == NULL)
        {
            printf("\nExiting...\n");
            break;
        }

        // 3. If they actually typed something, add it to the Up-Arrow history!
        if (strlen(input_buffer) > 0)
        {
            add_history(input_buffer);
        }
        else
        {
            // They just pressed Enter on an empty line
            free(input_buffer);
            continue;
        }

        // 4. Run your interpreter just like before
        reset_interpreter_line(&interpret, input_buffer);
        get_next_token(&interpret);

        ASTNode *tree = statement(&interpret);

        if (!interpret.error_found)
        {
            Value final_answer = evaluate(tree, &interpret);
            if (!interpret.error_found)
            {
                if (final_answer.type == VAL_INT)
                {
                    printf("%d\n", final_answer.as.i_val);
                }
                else if (final_answer.type == VAL_FLOAT)
                {
                    printf("%f\n", final_answer.as.f_val);
                }
            }
        }

        free_ast(tree);

        // 5. CRITICAL: You must free the string readline gave you!
        free(input_buffer);
    }

    free_interpreter(&interpret);
    return 0;
}
