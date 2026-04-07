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

// Ast node types
typedef enum
{
    NODE_NUM,
    NODE_BINOP,
    NODE_UNAOP
} ast_node_type;

// Ast nodes
typedef struct ASTNode 
{
    ast_node_type type;
    Token token;
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

// Expression function
ASTNode* expr(Interpreter* interpret);
// Evaluate the result
int evaluate(ASTNode* node, Interpreter* interpret);
// Clean up the ast in memory
void free_ast(ASTNode* node);

// Lexer function to get the next token in the input stream
void get_next_token(Interpreter* interpret);

#endif // INTERPRETER_H_
