#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
    VAL_INT,
    VAL_FLOAT
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        int i_val;
        double f_val;
    } as;
} Value;

// Available operators and EOL -> end of line
typedef enum
{
    NONE = 0,
    INT,
    FLOAT,
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
    Value value;
    bool is_const;
} Variable;

// Token struct
typedef struct
{
    Value value;
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

// Statement function
ASTNode *statement(Interpreter *interpret);
// Evaluate the result
Value evaluate(ASTNode *node, Interpreter *interpret);
// Clean up the ast in memory
void free_ast(ASTNode *node);

// Lexer function to get the next token in the input stream
void get_next_token(Interpreter *interpret);

// Initialize the interpreter struct
void init_interpreter(Interpreter *interpret);
void reset_interpreter_line(Interpreter *interpret, char *buffer);

// Free the allocated memory of the variable structur in the interpreter
void free_interpreter(Interpreter *interpret);
#endif // INTERPRETER_H_
