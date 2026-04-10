#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    ID,
    ASSIGN,
    EOL,
    SPACE,
    ERROR,
} token_types;

// Max length of the token name
#define NAME_LENGTH 11
typedef struct
{
    char name[NAME_LENGTH];
    int value;
} Variable;

// Token struct
typedef struct
{
    int value;
    char name[NAME_LENGTH];
    token_types type;
} Token;

typedef struct
{
    char *buffer;
    size_t length;
    size_t position;
    Token current_token;
    bool error_found;

    Variable *variables;
    unsigned int var_count;
    unsigned int var_capacity;
} Interpreter;

// Ast node types
typedef enum
{
    NODE_NUM,
    NODE_BINOP,
    NODE_UNAOP,
    NODE_ASSIGN,
    NODE_VAR
} ast_node_type;

// Ast nodes
typedef struct ASTNode
{
    ast_node_type type;
    Token token;
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

// Expression function
ASTNode *expr(Interpreter *interpret);
// Evaluate the result
int evaluate(ASTNode *node, Interpreter *interpret);
// Clean up the ast in memory
void free_ast(ASTNode *node);

// Lexer function to get the next token in the input stream
void get_next_token(Interpreter *interpret);

#endif // INTERPRETER_H_
