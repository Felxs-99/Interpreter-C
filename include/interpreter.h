#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Max length of the variable name
#define NAME_LENGTH 11

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

// Available token types
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
    BIT_OR,
    BIT_AND,
    BIT_XOR,
    BIT_NOT,
    CONST,
    EOL,
    SPACE,
    ERROR,
} TokenType;

// Token struct
typedef struct
{
    Value value;
    char name[NAME_LENGTH];
    TokenType type;
} Token;

// Represents a declared variable
typedef struct
{
    char name[NAME_LENGTH];
    bool is_const;
} Symbol;

// The Semantic Analyzer's memory, aka symbol table
typedef struct
{
    Symbol *symbols;
    unsigned int count;
    unsigned int capacity;
    bool error_found;
} SymbolTable;

// Represents a runtime value in memory
typedef struct
{
    char name[NAME_LENGTH];
    Value value;
} MemorySlot;

// Run-time evaluator
typedef struct
{
    char *buffer;
    size_t length;
    size_t position;
    Token current_token;
    bool error_found;

    MemorySlot *memory;
    unsigned int mem_count;
    unsigned int mem_capacity;
} Interpreter;

// Ast node types
typedef enum
{
    NODE_NUM,
    NODE_BINOP,
    NODE_UNAOP,
    NODE_ASSIGN,
    NODE_CONST_ASSIGN,
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

// Create a symbol table from the ast
void analyze_tree(ASTNode *node, SymbolTable *symtab);
// Init the symbol table
void init_symtab(SymbolTable *symtab);
// Free the allocated memory of the symbol table
void free_symtab(SymbolTable *symtab);

// Initialize the interpreter struct
void init_interpreter(Interpreter *interpret);
void reset_interpreter_line(Interpreter *interpret, char *buffer);

// Free the allocated memory of the variable structur in the interpreter
void free_interpreter(Interpreter *interpret);
#endif // INTERPRETER_H_
