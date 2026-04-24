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
#include <sys/stat.h>

static void cli();
static void file_interpreter(char *path);

int main(int argc, char *argv[])
{
    /* If an argument is given, it is assumed it is a path to a file that should
     * be interpreted, if it isn't the command line interface is launched*/
    if (argc > 1 && argv != NULL)
    {
        struct stat path_stat;
        if (stat(argv[1], &path_stat) == 0)
        {
            if (!S_ISREG(path_stat.st_mode))
            {
                printf("'%s' is a valid regular file.\n", argv[1]);
                return 1;
            }
        }
        else
        {
            printf("Error: Path '%s' does not exist.\n", argv[1]);
            return 1;
        }

        /* first one is the ./main! */
        file_interpreter(argv[1]);
    }
    else
    {
        cli();
    }
}

static void file_interpreter(char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    char buffer[256];

    Interpreter interpret = {0};
    init_interpreter(&interpret);

    SymbolTable symtab = {0};
    init_symtab(&symtab);

    while (fgets(buffer, sizeof(buffer), file))
    {
        char print_buffer[256];
        strcpy(print_buffer, buffer);
        print_buffer[strcspn(print_buffer, "\r\n")] = '\0';
        printf("%-30s -> ", print_buffer);

        // --- PHASE 1: PARSE (Build the tree first!) ---
        reset_interpreter_line(&interpret, buffer);
        get_next_token(&interpret);
        ASTNode *tree = statement(&interpret);

        if (interpret.error_found || tree == NULL)
        {
            free_ast(tree);
            break; // Stop if syntax is bad
        }

        // --- PHASE 2: SEMANTIC ANALYSIS (Check the rules) ---
        // Now you actually have a tree to pass in!
        analyze_tree(tree, &symtab);

        if (symtab.error_found)
        {
            free_ast(tree);
            symtab.error_found =
                false; // Reset the flag so the next line works!
            break;     // Stop if rules are broken (like "pi = 4")
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
    }

    fclose(file);

    free_interpreter(&interpret);
    free_symtab(&symtab);
}

// This is the command line interface of the interpreter
static void cli()
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
}
