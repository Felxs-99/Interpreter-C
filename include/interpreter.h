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
    VAL_FLOAT,
    VAL_BOOL
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        long long i_val;
        double f_val;
        bool b_val;
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
    LBRACE,
    RBRACE,
    ID,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    EQUAL_LESS,
    LESS,
    EQUAL_GREATER,
    GREATER,
    BIT_OR,
    BIT_AND,
    BIT_XOR,
    BIT_NOT,
    TRUE,
    FALSE,
    CONST,
    IF,
    ELSE,
    WHILE,
    FUNCTION,
    PRINT,
    EOL,
    EOF_TOKEN,
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
typedef struct SymbolTable
{
    Symbol *symbols;
    unsigned int count;
    unsigned int capacity;
    bool error_found;
    struct SymbolTable *enclosing_scope;
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
    NODE_LITERAL,
    NODE_BINOP,
    NODE_UNAOP,
    NODE_ASSIGN,
    NODE_CONST_ASSIGN,
    NODE_VAR,
    NODE_IF,
    NODE_COMPOUND,
    NODE_WHILE,
    NODE_FUNC_DEF,
    NODE_FUNC_CALL,
    NODE_PRINT
} ast_node_type;

// Ast nodes
typedef struct ASTNode
{
    ast_node_type type;
    Token token;
    struct ASTNode *left;
    struct ASTNode *right;
    union
    {
        struct ASTNode *else_node; // NODE_IF
        struct
        { // NODE_FUNC_DEF
            char *name;
            char **params;
            int param_count;
        } func_def;
        struct
        { // NODE_FUNC_CALL
            char *name;
            struct ASTNode **args;
            int arg_count;
        } func_call;
    } ext;
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
