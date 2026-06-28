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

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);

    Interpreter interpret = {0};
    init_interpreter(&interpret);

    SymbolTable symtab = {0};
    init_symtab(&symtab);
    fread(buffer, 1, file_size, file);
    // Null ternmination of the buffer
    buffer[file_size] = '\0';
    reset_interpreter_line(&interpret, buffer);
    get_next_token(&interpret);
    while (interpret.current_token.type != EOF_TOKEN && !interpret.error_found)
    {

        if (interpret.current_token.type == EOL)
        {
            get_next_token(&interpret);
            continue;
        }
        // --- PHASE 1: PARSE (Build the tree first!) ---
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

        evaluate(tree, &interpret, &symtab);

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
    long block_depth = 0;

    char *block_buffer = NULL;
    long block_size = 0;
    while (1)
    {
        char *input_buffer;
        // 1. readline handles the prompt AND reads the keystrokes!
        if (block_depth == 0)
        {
            input_buffer = readline(">>> ");
        }
        else
        {
            input_buffer = readline("... ");
        }
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

        for (int i = 0; input_buffer[i] != '\0'; i++)
        {
            if (input_buffer[i] == '{')
            {
                block_depth++;
            }
            if (input_buffer[i] == '}')
            {
                block_depth--;
            }
        }

        block_buffer =
            realloc(block_buffer, block_size + strlen(input_buffer) + 2);
        if (block_size == 0)
        {
            block_buffer[0] = '\0';
        }
        strcat(block_buffer, input_buffer);
        strcat(block_buffer, "\n");
        block_size += strlen(input_buffer) + 1;
        if (block_depth != 0)
        {
            continue;
        }
        // --- PHASE 1: PARSE (Build the tree first!) ---
        reset_interpreter_line(&interpret, block_buffer);
        get_next_token(&interpret);

        // --- PHASE 2: SEMANTIC ANALYSIS (Check the rules) ---
        while (interpret.current_token.type != EOF_TOKEN &&
               !interpret.error_found)
        {

            if (interpret.current_token.type == EOL)
            {
                get_next_token(&interpret);
                continue;
            }
            // --- PHASE 1: PARSE (Build the tree first!) ---
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

            evaluate(tree, &interpret, &symtab);

            // --- CLEANUP ---
            free_ast(tree);
        }
        free(input_buffer);
        free(block_buffer);
        block_buffer = NULL;
        block_size = 0;
        block_depth = 0;
    }
    free_interpreter(&interpret);
    free_symtab(&symtab);
}
