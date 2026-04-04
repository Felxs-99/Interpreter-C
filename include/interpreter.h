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
    MUL,
    DIV,
    LPAREN,
    RPAREN,
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

int calc(char* buffer, size_t length);
#endif // INTERPRETER_H_
