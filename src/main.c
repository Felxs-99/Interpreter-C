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
    Interpreter interpret = {0}; 
    interpret.buffer = input_buffer;
    interpret.length = strlen(input_buffer);
    interpret.position = 0;
    interpret.error_found = false;
    
    get_next_token(&interpret);
    ASTNode* tree = expr(&interpret);

    if (!interpret.error_found)
    {
      int final_answer = evaluate(tree, &interpret);

      if (!interpret.error_found)
      {
        printf("%d\n", final_answer);
      }
    }
    free_ast(tree);
  }
  return 0;
}
