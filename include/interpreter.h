#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Available operators and EOL -> end of line
typedef enum
{
    NONE = 0,
    INT,
    PLUS,
    MINUS,
    EOL,
    SPACE,
    ERROR,
} token_types;

// Token struct
typedef struct
{
  int value;
  token_types type;
} Token;

typedef struct
{
    char *buffer;
    size_t length;
    size_t position;
    Token current_token;
    bool error_found;
} Interpreter;


int expr(char* buffer, size_t lenght);
void get_next_token(Interpreter* interpret);
uint8_t convert_char(char character);
bool is_digit(char character);
bool eat(token_types token, Interpreter* interprete);


#endif // INTERPRETER_H_
