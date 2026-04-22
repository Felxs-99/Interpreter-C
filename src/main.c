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
    init_interpreter(&interpret);

    SymbolTable symtab = {0};
    init_symtab(&symtab);
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

        // --- PHASE 1: PARSE (Build the tree first!) ---
        reset_interpreter_line(&interpret, input_buffer);
        get_next_token(&interpret);
        ASTNode *tree = statement(&interpret);

        if (interpret.error_found || tree == NULL)
        {
            free_ast(tree);
            free(input_buffer);
            continue; // Stop if syntax is bad
        }

        // --- PHASE 2: SEMANTIC ANALYSIS (Check the rules) ---
        // Now you actually have a tree to pass in!
        analyze_tree(tree, &symtab);

        if (symtab.error_found)
        {
            free_ast(tree);
            free(input_buffer);
            symtab.error_found =
                false; // Reset the flag so the next line works!
            continue;  // Stop if rules are broken (like "pi = 4")
        }

        // --- PHASE 3: EVALUATE (Do the math) ---
        Value final_answer = evaluate(tree, &interpret);

        // Only print if evaluation didn't trigger a runtime error (like divide
        // by zero)
        if (!interpret.error_found)
        {
            if (final_answer.type == VAL_INT)
            {
                printf("%lld\n", final_answer.as.i_val);
            }
            else if (final_answer.type == VAL_FLOAT)
            {
                printf("%.7g\n", final_answer.as.f_val);
            }
            else if (final_answer.type == VAL_BOOL)
            {
                printf("%s\n", final_answer.as.b_val ? "true" : "false");
            }
        }

        // --- CLEANUP ---
        free_ast(tree);
        free(input_buffer);
    }

    free_interpreter(&interpret);
    free_symtab(&symtab);
    return 0;
}
