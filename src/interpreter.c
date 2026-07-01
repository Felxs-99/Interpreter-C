#include "interpreter.h"
#include "errno.h"
#include "overflow.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parser function prototypes
static ASTNode *create_literal_node(Token token);
static ASTNode *create_binop_node(ASTNode *left, Token op, ASTNode *right);
static ASTNode *create_unaop_node(Token op, ASTNode *right);
static ASTNode *create_assign_node(ASTNode *left, Token op, ASTNode *right);
static ASTNode *create_const_assign_node(ASTNode *left, Token op,
                                         ASTNode *right);
static ASTNode *create_var_node(Token id);
static ASTNode *create_if_node(ASTNode *condition, ASTNode *body,
                               ASTNode *else_node);
static ASTNode *create_compound_node(ASTNode *statement, ASTNode *next);
static ASTNode *create_while_node(ASTNode *condition, ASTNode *body);
static ASTNode *create_function_def_node(char *function_name, char **parameter,
                                         unsigned int parameter_count,
                                         ASTNode *body);
static ASTNode *create_function_call_node(char *function_name,
                                          ASTNode **arguments,
                                          unsigned int arguments_count);
static ASTNode *create_print_node(ASTNode *expr);
static ASTNode *create_return_node(ASTNode *expr);
static ASTNode *parse_id_or_call(Token id_token, Interpreter *interpret);

static ASTNode *clone_ast(ASTNode *node);
static bool eat(TokenType token, Interpreter *interprete);
static ASTNode *parse_function_def(Interpreter *interpret);
static ASTNode *parse_if(Interpreter *interpret);
static ASTNode *parse_while(Interpreter *interpret);
static ASTNode *parse_const(Interpreter *interpret);
static ASTNode *parse_print(Interpreter *interpret);
static ASTNode *bitwise_or_expr(Interpreter *interpret);
static ASTNode *bitwise_xor_expr(Interpreter *interpret);
static ASTNode *bitwise_and_expr(Interpreter *interpret);
static ASTNode *equality(Interpreter *interpret);
static ASTNode *relation(Interpreter *interpret);
static ASTNode *expr(Interpreter *interpret);
static ASTNode *term(Interpreter *interpret);
static ASTNode *factor(Interpreter *interpret);

// Lexer function prototypes
static TokenType get_keyword_type(const char *name);

// Symbol table function prototypes
static void define_symbol(SymbolTable *symtab, const char *name, bool is_const);
static void define_function(SymbolTable *symtab, const char *name,
                            unsigned int param_count, char **parameters,
                            ASTNode *body);
static bool grow_symtab(SymbolTable *symtab);
static bool lookup_symbol(SymbolTable *symtab, const char *name);
static bool lookup_function(SymbolTable *symtab, const char *name,
                            unsigned int arguments_count);
static bool is_buildin_function(const char *name);
static void init_builtin_symbols(SymbolTable *symtab);
static SymbolTable *create_child_scope(SymbolTable *parent_scope,
                                       char **parameters,
                                       unsigned int parameter_count);

// Interpreter function prototypes
static Value evaluate_binop(ASTNode *node, Interpreter *interpret,
                            SymbolTable *symtab);
static Value evaluate_unaop(ASTNode *node, Interpreter *interpret,
                            SymbolTable *symtab);
static Value evaluate_function_call(ASTNode *node, Interpreter *interpret,
                                    SymbolTable *symtab);
static RuntimeScope *init_scope(RuntimeScope *parent_scope);
static void free_scope(RuntimeScope *scope);
static void set_variable(Interpreter *interpret, const char *name, Value value,
                         bool is_local);
static Value get_variable(Interpreter *interpret, const char *name);
static void set_math_const(Interpreter *interpret);
static Symbol *get_function(SymbolTable *symtab, const char *name);

// Helper function prototypes
static void make_simple_token(Interpreter *interpret, TokenType type);
static void set_error_state_interpret(Interpreter *interpret);
static void set_error_state_symtab(SymbolTable *symtab);
static bool is_additive_op(TokenType type);
static bool is_multiplicative_op(TokenType type);
static bool is_assign_op(TokenType type);
static bool is_relation_op(TokenType type);
static char peek(Interpreter *interpret);
static ASTNode *parse_compound(Interpreter *interpret);
static void skip_eol(Interpreter *interpret);
static Value buildin_unary_math(char *func_name, Interpreter *interpret,
                                Value *arguments, unsigned int arguments_count,
                                double (*trigonomy_function)(double));
static Value buildin_sin(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count);
static Value buildin_asin(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count);
static Value buildin_cos(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count);
static Value buildin_acos(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count);
static Value buildin_tan(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count);
static Value buildin_atan(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count);

static Value buildin_sqrt(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count);
static Value buildin_floor(Interpreter *interpret, Value *arguments,
                           unsigned int arguments_count);

// Structure for math constants (to be in one place)
typedef struct
{
    const char *name;
    Value value;
} BuiltinConstant;

// The Single Source of Truth for all math constants!
static const BuiltinConstant BUILTIN_CONSTANTS[] = {
    {"e", {VAL_FLOAT, {.f_val = 2.718282}}},
    {"pi", {VAL_FLOAT, {.f_val = 3.141593}}},
    {"c", {VAL_INT, {.i_val = 299792458}}},
    {"ep", {VAL_FLOAT, {.f_val = 8.854188}}},
    {"mu", {VAL_FLOAT, {.f_val = 1.256637}}}};

// Calculate how many items are in the list automatically
static const unsigned int NUM_BUILTINS =
    sizeof(BUILTIN_CONSTANTS) / sizeof(BUILTIN_CONSTANTS[0]);

// Structure for buildin functions
typedef struct
{
    const char *name;
    unsigned int arguments_count;
    Value (*fn)(Interpreter *, Value *, unsigned int);
} BuiltinFunctions;

static const BuiltinFunctions BUILDIN_FUNCTIONS[] = {
    {
        "sin",
        1,
        buildin_sin,
    },
    {
        "asin",
        1,
        buildin_asin,
    },
    {
        "cos",
        1,
        buildin_cos,
    },
    {
        "acos",
        1,
        buildin_acos,
    },
    {
        "tan",
        1,
        buildin_tan,
    },
    {
        "atan",
        1,
        buildin_atan,
    },
    {
        "sqrt",
        1,
        buildin_sqrt,
    },
    {
        "floor",
        1,
        buildin_floor,
    },
};

static const unsigned int NUM_BUILTIN_FUNCTIONS =
    sizeof(BUILDIN_FUNCTIONS) / sizeof(BUILDIN_FUNCTIONS[0]);

typedef struct
{
    const char *name;
    TokenType type;
} Keyword;

static const Keyword RESERVED_KEYWORD[] = {
    {"const", CONST}, {"true", TRUE},    {"false", FALSE},
    {"if", IF},       {"else", ELSE},    {"print", PRINT},
    {"while", WHILE}, {"def", FUNCTION}, {"return", RETURN}};

static const unsigned int NUM_RESERVED_KEYWORDS =
    sizeof(RESERVED_KEYWORD) / sizeof(RESERVED_KEYWORD[0]);

/*
 * ####################
 * #     PARSER       #
 * ####################
 */

// Ast node for numbers
static ASTNode *create_literal_node(Token token)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_LITERAL;
    node->token = token;
    node->left = NULL;
    node->right = NULL;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for binary operations
static ASTNode *create_binop_node(ASTNode *left, Token op, ASTNode *right)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_BINOP;
    node->token = op;
    node->left = left;
    node->right = right;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for unary operations
static ASTNode *create_unaop_node(Token op, ASTNode *expr)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_UNAOP;
    node->token = op;
    node->left = NULL;
    node->right = expr;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for assign operators
static ASTNode *create_assign_node(ASTNode *left, Token op, ASTNode *right)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_ASSIGN;
    node->token = op;
    node->left = left;
    node->right = right;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for const assign operators
static ASTNode *create_const_assign_node(ASTNode *left, Token op,
                                         ASTNode *right)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_CONST_ASSIGN;
    node->token = op;
    node->left = left;
    node->right = right;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for variables (ids)
static ASTNode *create_var_node(Token id)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_VAR;
    node->token = id;
    node->left = NULL;
    node->right = NULL;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for if statement
static ASTNode *create_if_node(ASTNode *condition, ASTNode *body,
                               ASTNode *else_node)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_IF;
    node->token = (Token){0};
    node->left = condition;
    node->right = body;
    node->ext.else_node = else_node;
    return node;
}

// Ast node for statement compound
static ASTNode *create_compound_node(ASTNode *statement, ASTNode *next)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_COMPOUND;
    node->token = (Token){0};
    node->left = statement;
    node->right = next;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for while loop
static ASTNode *create_while_node(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_WHILE;
    node->token = (Token){0};
    node->left = condition;
    node->right = body;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for function definition
static ASTNode *create_function_def_node(char *function_name, char **parameter,
                                         unsigned int parameter_count,
                                         ASTNode *body)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_FUNC_DEF;
    node->token = (Token){0};
    node->left = body;
    node->right = NULL;
    node->ext.func_def.name = function_name;
    node->ext.func_def.params = parameter;
    node->ext.func_def.param_count = parameter_count;
    return node;
}

// Ast node for function call
static ASTNode *create_function_call_node(char *function_name,
                                          ASTNode **arguments,
                                          unsigned int arguments_count)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_FUNC_CALL;
    node->token = (Token){0};
    node->left = NULL;
    node->right = NULL;
    node->ext.func_call.name = function_name;
    node->ext.func_call.args = arguments;
    node->ext.func_call.arg_count = arguments_count;
    return node;
}

// Ast node for print
static ASTNode *create_print_node(ASTNode *expr)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_PRINT;
    node->token = (Token){0};
    node->left = expr;
    node->right = NULL;
    node->ext.else_node = NULL;
    return node;
}

// Ast node for print
static ASTNode *create_return_node(ASTNode *expr)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_RETURN;
    node->token = (Token){0};
    node->left = expr;
    node->right = NULL;
    node->ext.else_node = NULL;
    return node;
}

// Clean up for ast
void free_ast(ASTNode *node)
{
    if (node == NULL)
        return;
    free_ast(node->left); // Free children first (Post-order traversal)
    free_ast(node->right);

    switch (node->type)
    {
        case NODE_FUNC_DEF:
            free(node->ext.func_def.name);
            for (unsigned int i = 0; i < node->ext.func_def.param_count; i++)
            {
                free(node->ext.func_def.params[i]);
            }
            free(node->ext.func_def.params);
            break;
        case NODE_FUNC_CALL:
            free(node->ext.func_call.name);

            for (unsigned int i = 0; i < node->ext.func_call.arg_count; i++)
            {
                free_ast(node->ext.func_call
                             .args[i]); // args is an array of ast nodes
            }
            free(node->ext.func_call.args);
            break;
        default:
            break;
    }
    free(node); // Then free the parent
}

// Clone a ast node to another memory slot
static ASTNode *clone_ast(ASTNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    ASTNode *copied_node = (ASTNode *)malloc(sizeof(ASTNode));
    *copied_node = *node;
    copied_node->left = clone_ast(node->left);
    copied_node->right = clone_ast(node->right);

    // Copy the ext stuff in a node, but only if it is used in the union
    switch (node->type)
    {
        case NODE_FUNC_DEF:
            copied_node->ext.func_def.name = strdup(node->ext.func_def.name);
            copied_node->ext.func_def.param_count =
                node->ext.func_def.param_count;
            copied_node->ext.func_def.params =
                malloc(node->ext.func_def.param_count * sizeof(char *));

            for (unsigned int i = 0; i < node->ext.func_def.param_count; i++)
            {
                copied_node->ext.func_def.params[i] =
                    strdup(node->ext.func_def.params[i]);
            }
            break;
        case NODE_FUNC_CALL:
            copied_node->ext.func_call.name = strdup(node->ext.func_call.name);
            copied_node->ext.func_call.arg_count =
                node->ext.func_call.arg_count;
            copied_node->ext.func_call.args =
                malloc(node->ext.func_call.arg_count * sizeof(ASTNode *));

            for (unsigned int i = 0; i < node->ext.func_call.arg_count; i++)
            {
                copied_node->ext.func_call.args[i] =
                    clone_ast(node->ext.func_call.args[i]);
            }
            break;
        default:
            break;
    }
    return copied_node;
}

// Eat the provided token
static bool eat(TokenType token, Interpreter *interpret)
{
    if (interpret->current_token.type == token)
    {
        get_next_token(interpret);
        return true;
    }
    return false;
}

// Helper function to parse the function keyword
static ASTNode *parse_function_def(Interpreter *interpret)
{
    if (!eat(FUNCTION, interpret))
    {
        printf("Syntax Error: Expected keyword 'def'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    // Grab the name of the function
    char *func_name = NULL;
    if (interpret->current_token.type == ID)
    {
        func_name = strdup(interpret->current_token.name);
        eat(ID, interpret);
    }
    else
    {
        printf("Syntax Error: Expected a function name after 'def'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    if (!eat(LPAREN, interpret))
    {
        printf("Syntax Error: Expected '(' after function name.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    // Parse parameter list
    char **parameter_name = NULL;
    int parameter_count = 0;
    while (interpret->current_token.type == ID)
    {
        parameter_name =
            realloc(parameter_name, (parameter_count + 1) * sizeof(char *));
        parameter_name[parameter_count] = strdup(interpret->current_token.name);
        parameter_count++;
        eat(ID, interpret); // no need for eat check, because loop does it

        // Check if there is a comma after the parameter
        if (eat(COMMA, interpret))
        {
            if (interpret->current_token.type != ID)
            {
                printf("Syntax Error: Expected parameter name after ','\n");
                set_error_state_interpret(interpret);
                goto cleanup;
            }
        }
    }
    if (!eat(RPAREN, interpret))
    {
        printf("Syntax Error: Expected ')' after function parameters.\n");
        set_error_state_interpret(interpret);
        goto cleanup;
    }

    // Parse the body
    skip_eol(interpret);
    if (!eat(LBRACE, interpret))
    {
        printf(
            "Syntax Error: Expected '{' for the start of a function body.\n");
        set_error_state_interpret(interpret);
        goto cleanup;
    }
    ASTNode *left_node = parse_compound(interpret);
    if (!eat(RBRACE, interpret))
    {
        printf("Syntax Error: Expected '}' for the end of a function body.\n");
        set_error_state_interpret(interpret);
        goto cleanup;
    }

    return create_function_def_node(func_name, parameter_name, parameter_count,
                                    left_node);

// Clean up lable
cleanup:
    for (int i = 0; i < parameter_count; i++)
        free(parameter_name[i]);
    free(parameter_name);
    free(func_name);
    return NULL;
}

// Helper function to parse a function call
static ASTNode *parse_function_call(char *id_name, Interpreter *interpret)
{
    char *func_name = strdup(id_name);
    if (!eat(LPAREN, interpret))
    {
        printf(
            "Syntax Error: Expected a '(' at the start of the argumet list.\n");
        set_error_state_interpret(interpret);
        free(func_name);
        return NULL;
    }

    // Parse argument list
    ASTNode **argument_list = NULL;
    int argumet_count = 0;
    while (interpret->current_token.type != RPAREN &&
           interpret->current_token.type != EOF_TOKEN)
    {
        skip_eol(interpret);
        argument_list =
            realloc(argument_list, (argumet_count + 1) * sizeof(ASTNode *));
        argument_list[argumet_count] = bitwise_or_expr(interpret);
        argumet_count++;

        // Check if there is a comma after the parameter
        if (eat(COMMA, interpret))
        {
            if (interpret->current_token.type == RPAREN ||
                interpret->current_token.type == EOF_TOKEN)
            {
                printf("Syntax Error: Expected parameter name after ','\n");
                set_error_state_interpret(interpret);
                goto cleanup;
            }
        }
    }

    if (!eat(RPAREN, interpret))
    {
        printf(
            "Syntax Error: Expected a ')' at the end of the argumet list.\n");
        set_error_state_interpret(interpret);
        goto cleanup;
    }

    return create_function_call_node(func_name, argument_list, argumet_count);

// Clean up lable
cleanup:
    for (int i = 0; i < argumet_count; i++)
        free(argument_list[i]);
    free(argument_list);
    free(func_name);
    return NULL;
}

// Helper function to parse the if keyword
static ASTNode *parse_if(Interpreter *interpret)
{
    if (!eat(IF, interpret))
    {
        printf("Syntax Error: Expected keyword 'if'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    // The left node should be a boolean expression
    ASTNode *left_node = bitwise_or_expr(interpret);
    skip_eol(interpret);
    if (!eat(LBRACE, interpret))
    {
        printf("Syntax Error: Expected '{' for the start of a if statement "
               "body.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    ASTNode *right_node = parse_compound(interpret);
    if (!eat(RBRACE, interpret))
    {
        printf("Syntax Error: Expected '}' for the end of a if statement "
               "body.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    skip_eol(interpret);
    ASTNode *else_node = NULL;

    if (eat(ELSE, interpret))
    {
        skip_eol(interpret);
        if (!eat(LBRACE, interpret))
        {
            printf("Syntax Error: Expected '{' for the start of a if statement "
                   "body.\n");
            set_error_state_interpret(interpret);
            return NULL;
        }
        else_node = parse_compound(interpret);
        skip_eol(interpret);
        if (!eat(RBRACE, interpret))
        {
            printf("Syntax Error: Expected '}' for the end of a if statement "
                   "body.\n");
            set_error_state_interpret(interpret);
            return NULL;
        }
    }
    return create_if_node(left_node, right_node, else_node);
}

// Helper function to parse the while keyword
static ASTNode *parse_while(Interpreter *interpret)
{
    if (!eat(WHILE, interpret))
    {
        printf("Syntax Error: Expected keyword 'while'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    // The left node should be a boolean expression
    ASTNode *left_node = bitwise_or_expr(interpret);
    skip_eol(interpret);
    if (!eat(LBRACE, interpret))
    {
        printf("Syntax Error: Expected '{' for the start of a while loop "
               "body.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    ASTNode *right_node = parse_compound(interpret);
    if (!eat(RBRACE, interpret))
    {
        printf("Syntax Error: Expected '}' for the end of a while loop "
               "body.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    skip_eol(interpret);

    return create_while_node(left_node, right_node);
}

// Helper function to parse the const keyword
static ASTNode *parse_const(Interpreter *interpret)
{
    if (!eat(CONST, interpret))
    {
        printf("Syntax Error: Expected keyword 'const'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    Token id_token = interpret->current_token;
    if (!eat(ID, interpret))
    {
        printf("Syntax Error: Expected variable name after 'const'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    ASTNode *left_node = create_var_node(id_token);

    Token assign_token = interpret->current_token;

    if (!eat(ASSIGN, interpret))
    {
        printf("Syntax Error: Expected '=' after constant name.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    ASTNode *right_node = bitwise_or_expr(interpret);

    return create_const_assign_node(left_node, assign_token, right_node);
}

// Helper function to parse the print keyword
static ASTNode *parse_print(Interpreter *interpret)
{
    if (!eat(PRINT, interpret))
    {
        printf("Syntax Error: Expected keyword 'print'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    if (!eat(LPAREN, interpret))
    {
        printf("Syntax Error: Expected '(' after the 'print' keyword.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    ASTNode *left_node = bitwise_or_expr(interpret);
    if (!eat(RPAREN, interpret))
    {
        printf(
            "Syntax Error: Expected ')' at the end of the 'print' keyword.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    return create_print_node(left_node);
}

// Helper function to parse the print keyword
static ASTNode *parse_return(Interpreter *interpret)
{
    if (!eat(RETURN, interpret))
    {
        printf("Syntax Error: Expected keyword 'return'.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    if (!eat(LPAREN, interpret))
    {
        printf("Syntax Error: Expected '(' after the 'return' keyword.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }

    // Check if something in the return statement
    if (interpret->current_token.type == RPAREN)
    {
        eat(RPAREN, interpret);
        return create_return_node(NULL);
    }

    ASTNode *left_node = bitwise_or_expr(interpret);
    if (!eat(RPAREN, interpret))
    {
        printf(
            "Syntax Error: Expected ')' at the end of the 'return' keyword.\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
    return create_return_node(left_node);
}

static ASTNode *parse_id_or_call(Token id_token, Interpreter *interpret)
{
    if (interpret->current_token.type == LPAREN)
    {
        return parse_function_call(id_token.name, interpret);
    }
    else
    {
        return create_var_node(id_token);
    }
}

ASTNode *statement(Interpreter *interpret)
{
    // Parsing the different keywords
    switch (interpret->current_token.type)
    {
        case FUNCTION:
            return parse_function_def(interpret);
        case IF:
            return parse_if(interpret);
        case WHILE:
            return parse_while(interpret);
        case CONST:
            return parse_const(interpret);
        case PRINT:
            return parse_print(interpret);
        case RETURN:
            return parse_return(interpret);
        default:
            break;
    }

    ASTNode *left_node = bitwise_or_expr(interpret);

    // Stop evaluating if a syntax error was found in expr()
    if (interpret->error_found)
    {
        free_ast(left_node);
        return NULL;
    }
    // Parse the assigment operators
    if (is_assign_op(interpret->current_token.type))
    {
        if (left_node->type != NODE_VAR)
        {
            printf("Syntax Error: You can only assign values to variables.\n");
            set_error_state_interpret(interpret);
            free_ast(left_node);
            return NULL;
        }
        Token token = interpret->current_token;
        if (!eat(ASSIGN, interpret))
        {
            printf("Syntax Error: Only values can be assigned to variables\n");
            set_error_state_interpret(interpret);
            free_ast(left_node);
            return NULL;
        }

        ASTNode *right_node = bitwise_or_expr(interpret);

        return create_assign_node(left_node, token, right_node);
    }

    // No assigment was used, just return the node from expr
    return left_node;
}

// Evaluate the bitwise or expression
static ASTNode *bitwise_or_expr(Interpreter *interpret)
{

    ASTNode *left_node = bitwise_xor_expr(interpret);
    Token token = {0};

    while (interpret->current_token.type == BIT_OR)
    {
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = bitwise_xor_expr(interpret);
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// Evaluate the bitwise xor expression
static ASTNode *bitwise_xor_expr(Interpreter *interpret)
{
    ASTNode *left_node = bitwise_and_expr(interpret);
    Token token = {0};

    while (interpret->current_token.type == BIT_XOR)
    {
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = bitwise_and_expr(interpret);
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// Evaluate the bitwise and expression
static ASTNode *bitwise_and_expr(Interpreter *interpret)
{
    ASTNode *left_node = equality(interpret);
    Token token = {0};

    while (interpret->current_token.type == BIT_AND)
    {
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = equality(interpret);
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// Evaluate equality expressions
static ASTNode *equality(Interpreter *interpret)
{
    ASTNode *left_node = relation(interpret);
    Token token = {0};

    while (interpret->current_token.type == EQUAL ||
           interpret->current_token.type == NOT_EQUAL)
    {
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = relation(interpret);
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// Evaluate relation expressions
static ASTNode *relation(Interpreter *interpret)
{
    ASTNode *left_node = expr(interpret);
    Token token = {0};

    while (is_relation_op(interpret->current_token.type))
    {
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = expr(interpret);
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}
// Evaluate the expression
static ASTNode *expr(Interpreter *interpret)
{
    ASTNode *left_node = term(interpret);
    Token token = {0};

    while (is_additive_op(interpret->current_token.type))
    {
        // Stop evaluating if a syntax error was found in factor()
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }
        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = term(interpret);

        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// term : factor ((MUL | DIV) factor)*
static ASTNode *term(Interpreter *interpret)
{
    ASTNode *left_node = factor(interpret);
    Token token = {0};

    while (is_multiplicative_op(interpret->current_token.type))
    {
        // Stop evaluating if a syntax error was found in factor()
        if (interpret->error_found)
        {
            free_ast(left_node);
            return NULL;
        }

        token = interpret->current_token;
        eat(token.type, interpret);

        ASTNode *right_node = factor(interpret);

        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// factor : (PLUS | MINUS) factor | INTEGER | LPAREN expr RPAREN
static ASTNode *factor(Interpreter *interpret)
{
    Token token = interpret->current_token;
    // For unary operators
    if ((token.type == PLUS) || (token.type == MINUS) ||
        (token.type == BIT_NOT))
    {
        eat(token.type, interpret);
        ASTNode *result = factor(interpret);
        return create_unaop_node(token, result);
    }
    // For integer values
    else if (token.type == INT)
    {
        eat(INT, interpret);
        return create_literal_node(token);
    }
    // For decimal values
    else if (token.type == FLOAT)
    {
        eat(FLOAT, interpret);
        return create_literal_node(token);
    }
    else if (token.type == TRUE || token.type == FALSE)
    {
        eat(token.type, interpret);
        return create_literal_node(token);
    }
    // For parentheses
    else if (token.type == LPAREN)
    {

        eat(LPAREN, interpret);
        ASTNode *result = bitwise_or_expr(interpret);
        if (!eat(RPAREN, interpret))
        {
            printf("Syntax Error: Missing closing ')'\n");
            set_error_state_interpret(interpret);
            free_ast(result);
            return NULL;
        }
        return result;
    }
    else if (token.type == ERROR)
    {
        interpret->error_found = true;
        return NULL;
    }
    else if (token.type == ID)
    {
        eat(ID, interpret);
        return parse_id_or_call(token, interpret);
    }
    else
    {
        printf("Syntax Error: Expected an Integer, an unary operator or "
               "'('\n");
        set_error_state_interpret(interpret);
        return NULL;
    }
}

/*
 * ####################
 * #     LEXER        #
 * ####################
 */

// Create the token for the next character
void get_next_token(Interpreter *interpret)
{
    // Create an empty token
    Token token = {0};
    char current_char = interpret->buffer[interpret->position];

    // Detect whitespaces and skip them, relies on the fact that buffer is 0
    // terminated
    while (current_char == ' ')
    {
        interpret->position++;
        current_char = interpret->buffer[interpret->position];
    }

    // Check if its a digit
    if (isdigit(current_char) || (current_char == '.'))
    {
        char temp_num[64] = {0};
        unsigned int temp_pos = 0;
        bool has_decimal = false;
        bool has_exponent = false;

        // Check if a binary or hex number provided
        if (current_char == '0')
        {
            char next_char = peek(interpret);

            if (next_char == 'b')
            {
                interpret->position += 2;

                while (interpret->buffer[interpret->position] == '0' ||
                       interpret->buffer[interpret->position] == '1')
                {
                    if (temp_pos < 63)
                    {
                        temp_num[temp_pos++] =
                            interpret->buffer[interpret->position++];
                    }
                }

                // Check for invalid characters after the binary number
                if (isalnum(interpret->buffer[interpret->position]))
                {
                    printf("Lexical Error: Invalid character '%c' in binary "
                           "literal.\n",
                           interpret->buffer[interpret->position]);
                    set_error_state_interpret(interpret);
                    return;
                }

                errno = 0;
                token.type = INT;
                token.value.type = VAL_INT;
                token.value.as.i_val = strtoll(temp_num, NULL, 2);

                // Check for overflow errors
                if (errno == ERANGE)
                {
                    printf("Lexical Error: Number '%s' is too large to "
                           "store!\n",
                           temp_num);
                    set_error_state_interpret(interpret);
                    return;
                }
                interpret->current_token = token;
                return;
            }
            else if (next_char == 'x')
            {
                interpret->position += 2;

                while (isxdigit(interpret->buffer[interpret->position]))
                {
                    if (temp_pos < 63)
                    {
                        temp_num[temp_pos++] =
                            interpret->buffer[interpret->position++];
                    }
                }

                // Check for invalid characters after the hex number
                if (isalpha(interpret->buffer[interpret->position]))
                {
                    printf("Lexical Error: Invalid character '%c' in hex "
                           "literal.\n",
                           interpret->buffer[interpret->position]);
                    set_error_state_interpret(interpret);
                    return;
                }

                errno = 0;
                token.type = INT;
                token.value.type = VAL_INT;
                token.value.as.i_val = strtoll(temp_num, NULL, 16);

                // Check for overflow errors
                if (errno == ERANGE)
                {
                    printf("Lexical Error: Number '%s' is too large to "
                           "store!\n",
                           temp_num);
                    set_error_state_interpret(interpret);
                    return;
                }
                interpret->current_token = token;
                return;
            }
        }

        // Get every following digit and make it one number
        while (isdigit(interpret->buffer[interpret->position]) ||
               interpret->buffer[interpret->position] == '.' ||
               interpret->buffer[interpret->position] == 'e' ||
               interpret->buffer[interpret->position] == 'E')
        {
            char c = interpret->buffer[interpret->position];

            // Check for decimal point
            if (c == '.')
            {
                if (has_decimal && !has_exponent)
                {
                    printf("Syntax Error: Multiple decimal points!\n");
                    set_error_state_interpret(interpret);
                    return;
                }

                if (has_decimal && has_exponent)
                {
                    printf("Syntax Error: Decimal point is not allowed in the "
                           "exponent!\n");
                    set_error_state_interpret(interpret);
                    return;
                }

                has_decimal = true;
            }
            else if (c == 'e' || c == 'E')
            {
                if (has_exponent)
                {
                    printf("Syntax Error: Multiple exponets detected!\n");
                    set_error_state_interpret(interpret);
                    return;
                }

                has_exponent = true;
                has_decimal = true;
            }
            // Prevent temp buffer overflow
            if (temp_pos >= (63))
            {
                printf("Value Error: Given Number is too long!\n");
                set_error_state_interpret(interpret);
                return;
            }

            temp_num[temp_pos++] = c;
            if (c == 'e' || c == 'E')
            {
                if (peek(interpret) == '+' || peek(interpret) == '-')
                {
                    interpret->position++;
                    temp_num[temp_pos++] =
                        interpret->buffer[interpret->position];
                }
            }
            interpret->position++;
        }

        temp_num[temp_pos] = '\0';
        errno = 0;
        if (has_decimal || has_exponent)
        {
            token.type = FLOAT;
            token.value.type = VAL_FLOAT;
            token.value.as.f_val = strtod(temp_num, NULL);
        }
        else
        {
            token.type = INT;
            token.value.type = VAL_INT;
            token.value.as.i_val = strtoll(temp_num, NULL, 10);
        }

        // Check for overflow errors
        if (errno == ERANGE)
        {
            printf("Lexical Error: Decimal number '%s' is out of range!\n",
                   temp_num);
            set_error_state_interpret(interpret);
            return;
        }
        interpret->current_token = token;
        return;
    }

    // Check if its a letter
    if (isalpha(current_char))
    {
        int position = 0;
        token.name[position] = current_char;
        interpret->position++;

        while (isalpha(interpret->buffer[interpret->position]))
        {
            position++;
            // Check for max allowed length - the \0
            if (position >= (NAME_LENGTH - 1))
            {
                printf("Syntax Error: Max length for variables are %d\n",
                       (NAME_LENGTH - 1));
                set_error_state_interpret(interpret);
                return;
            }

            token.name[position] = interpret->buffer[interpret->position];
            interpret->position++;
        }

        if (position >= (NAME_LENGTH - 2))
        {
            printf("Syntax Error: Max length for variables are %d\n",
                   (NAME_LENGTH - 1));
            set_error_state_interpret(interpret);
            return;
        }
        // Add traling \0
        token.name[++position] = '\0';
        token.type = get_keyword_type(token.name);

        // Check if the found keyword is a boolean one
        if (token.type == TRUE)
        {
            token.value.type = VAL_BOOL;
            token.value.as.b_val = true;
        }
        else if (token.type == FALSE)
        {
            token.value.type = VAL_BOOL;
            token.value.as.b_val = false;
        }
        interpret->current_token = token;
        return;
    }

    // Check if its a assign or equal character
    if (current_char == '=')
    {
        if (peek(interpret) == '=')
        {
            // make_simple_token also increases by 1 -> 2
            interpret->position++;
            make_simple_token(interpret, EQUAL);
            return;
        }
        else
        {
            make_simple_token(interpret, ASSIGN);
            return;
        }
    }
    // Check if its a not equal character
    if (current_char == '!')
    {
        if (peek(interpret) == '=')
        {
            // make_simple_token also increases by 1 -> 2
            interpret->position++;
            make_simple_token(interpret, NOT_EQUAL);
            return;
        }
    }

    // Check for equal greater and equal less
    if (current_char == '<')
    {
        if (peek(interpret) == '=')
        {
            // make_simple_token also increases by 1 -> 2
            interpret->position++;
            make_simple_token(interpret, EQUAL_LESS);
            return;
        }
        else
        {
            make_simple_token(interpret, LESS);
            return;
        }
    }

    if (current_char == '>')
    {
        if (peek(interpret) == '=')
        {
            // make_simple_token also increases by 1 -> 2
            interpret->position++;
            make_simple_token(interpret, EQUAL_GREATER);
            return;
        }
        else
        {
            make_simple_token(interpret, GREATER);
            return;
        }
    }
    // Check for known symbols and create the correspondig token
    switch (current_char)
    {
        case '+':
            make_simple_token(interpret, PLUS);
            return;
        case '-':
            make_simple_token(interpret, MINUS);
            return;
        case '*':
            make_simple_token(interpret, MUL);
            return;
        case '/':
            make_simple_token(interpret, DIV);
            return;
        case '(':
            make_simple_token(interpret, LPAREN);
            return;
        case ')':
            make_simple_token(interpret, RPAREN);
            return;
        case '{':
            make_simple_token(interpret, LBRACE);
            return;
        case '}':
            make_simple_token(interpret, RBRACE);
            return;
        case '|':
            make_simple_token(interpret, BIT_OR);
            return;
        case '&':
            make_simple_token(interpret, BIT_AND);
            return;
        case '^':
            make_simple_token(interpret, BIT_XOR);
            return;
        case '~':
            make_simple_token(interpret, BIT_NOT);
            return;
        case ',':
            make_simple_token(interpret, COMMA);
            return;
        case '\n':
            make_simple_token(interpret, EOL);
            return;
        case '\0':
            make_simple_token(interpret, EOF_TOKEN);
            return;
        default:
            break;
    }

    // Unknow character
    printf("Syntax Error: Unknown character '%c'\n", current_char);
    set_error_state_interpret(interpret);
    return;
}

// Check if the token name matches a reserved keyword
static TokenType get_keyword_type(const char *name)
{
    for (unsigned int i = 0; i < NUM_RESERVED_KEYWORDS; i++)
    {
        if (strcmp(name, RESERVED_KEYWORD[i].name) == 0)
        {
            return RESERVED_KEYWORD[i].type;
        }
    }

    return ID;
}

/*
 * ####################
 * #Semantic Analysis #
 * ####################
 */

// Create a symbol table from the ast
void analyze_tree(ASTNode *node, SymbolTable *symtab)
{
    if (node == NULL || symtab->error_found)
        return;

    switch (node->type)
    {
        case NODE_BINOP:
            analyze_tree(node->left, symtab);
            analyze_tree(node->right, symtab);
            break;
        case NODE_UNAOP:
            analyze_tree(node->right, symtab);
            break;
        case NODE_ASSIGN:
            // Check if variable exists, or add it to
            //  SymbolTable
            analyze_tree(node->right, symtab);
            define_symbol(symtab, node->left->token.name, false);
            break;

        case NODE_CONST_ASSIGN:
            // Check if const variable exists, or add it to
            //  SymbolTable
            analyze_tree(node->right, symtab);
            define_symbol(symtab, node->left->token.name, true);
            break;
        case NODE_VAR:
            // Look up the variable in the SymbolTable to ensure it was
            // declared!
            lookup_symbol(symtab, node->token.name);
            break;
        case NODE_LITERAL:
            // ignore number nodes for the moment
            break;
        case NODE_IF:
            analyze_tree(node->left, symtab);
            analyze_tree(node->right, symtab);
            analyze_tree(node->ext.else_node, symtab);
            break;
        case NODE_COMPOUND:
            analyze_tree(node->left, symtab);
            analyze_tree(node->right, symtab);
            break;
        case NODE_WHILE:
            analyze_tree(node->left, symtab);
            analyze_tree(node->right, symtab);
            break;
        case NODE_FUNC_DEF:
            define_function(symtab, node->ext.func_def.name,
                            node->ext.func_def.param_count,
                            node->ext.func_def.params, node->left);
            SymbolTable *child_scope =
                create_child_scope(symtab, node->ext.func_def.params,
                                   node->ext.func_def.param_count);
            // Error message should be printed already, just propagate to parent
            // symtab and brake operation
            if (child_scope == NULL)
            {
                symtab->error_found = true;
                break;
            }
            analyze_tree(node->left, child_scope);
            // Same thing, propagate to parent scope
            if (child_scope == NULL)
            {
                symtab->error_found = true;
                break;
            }
            free_symtab(child_scope);
            free(child_scope);
            break;
        case NODE_FUNC_CALL:
            // Check if its a builin function
            for (unsigned int i = 0; i < NUM_BUILTIN_FUNCTIONS; i++)
            {
                if (strcmp(BUILDIN_FUNCTIONS[i].name,
                           node->ext.func_call.name) == 0)
                {
                    // found it, check arguments list
                    for (unsigned int j = 0; j < node->ext.func_call.arg_count;
                         j++)
                    {
                        analyze_tree(node->ext.func_call.args[j], symtab);
                    }
                    if (BUILDIN_FUNCTIONS[i].arguments_count !=
                        node->ext.func_call.arg_count)
                    {
                        printf("Semantic Error: Builtin %s function takes %d "
                               "arguments, "
                               "but "
                               "got %d!\n",
                               BUILDIN_FUNCTIONS[i].name,
                               BUILDIN_FUNCTIONS[i].arguments_count,
                               node->ext.func_call.arg_count);
                        set_error_state_symtab(symtab);
                        return;
                    }

                    return;
                }
            }
            lookup_function(symtab, node->ext.func_call.name,
                            node->ext.func_call.arg_count);
            // Check everything in the arguments list
            for (unsigned int i = 0; i < node->ext.func_call.arg_count; i++)
            {
                analyze_tree(node->ext.func_call.args[i], symtab);
            }
            break;

        case NODE_PRINT:
            analyze_tree(node->left, symtab);
            break;
        case NODE_RETURN:
            analyze_tree(node->left, symtab);
            break;
        default:
            break;
    }
}

// Put a new symbol in the table if it does not exist or is not  const
static void define_symbol(SymbolTable *symtab, const char *name, bool is_const)
{
    // Check if the given name is a buildin function
    if (is_buildin_function(name))
    {
        printf("Semantic Error: '%s' is a built-in function and cannot be "
               "overwritten!\n",
               name);
        set_error_state_symtab(symtab);
        return;
    }
    // Check if the syymbol already exists
    for (unsigned int i = 0; i < symtab->count; i++)
    {
        if (strcmp(name, symtab->symbols[i].name) == 0)
        {
            if (symtab->symbols[i].is_const)
            {
                printf("Semantic Error: Cannot reassign constant '%s'\n", name);
                set_error_state_symtab(symtab);
            }

            else if (is_const)
            {
                printf("Semantic Error: Cannot redeclare existing variable "
                       "'%s' as a constant\n",
                       name);
                set_error_state_symtab(symtab);
            }

            return;
        }
    }
    // Check if the symol table need to grow
    if (!grow_symtab(symtab))
    {
        return;
    }

    unsigned int index = symtab->count;

    if (strlen(name) < NAME_LENGTH)
    {
        strcpy(symtab->symbols[index].name, name);
        symtab->symbols[index].is_const = is_const;
        symtab->symbols[index].symbol_kind = KIND_VAR;
        memset(&symtab->symbols[index].ext, 0,
               sizeof(symtab->symbols[index].ext));
        symtab->count++;
    }
    else
    {
        printf("Semantic Error: Name of variable too long!\n ");
        set_error_state_symtab(symtab);
    }
}

// Put a new function in the table if it does not exist or is not const
static void define_function(SymbolTable *symtab, const char *name,
                            unsigned int param_count, char **parameters,
                            ASTNode *body)
{
    // Check if the given name is a buildin function
    if (is_buildin_function(name))
    {
        printf("Semantic Error: '%s' is a built-in function and cannot be "
               "redeclared!\n",
               name);
        set_error_state_symtab(symtab);
        return;
    }
    // Check if the syymbol already exists
    for (unsigned int i = 0; i < symtab->count; i++)
    {
        if (strcmp(name, symtab->symbols[i].name) == 0)
        {
            if (symtab->symbols[i].is_const)
            {
                if (symtab->symbols[i].symbol_kind != KIND_FUNC)
                {
                    printf("Semantic Error: '%s' is not a function!\n", name);
                    set_error_state_symtab(symtab);
                }
                else
                {
                    printf(
                        "Semantic Error: Cannot reassign function name! '%s'\n",
                        name);
                    set_error_state_symtab(symtab);
                }
            }

            return;
        }
    }
    // Check if the symol table need to grow
    if (!grow_symtab(symtab))
    {
        return;
    }

    unsigned int index = symtab->count;

    if (strlen(name) < NAME_LENGTH)
    {
        strcpy(symtab->symbols[index].name, name);
        symtab->symbols[index].symbol_kind = KIND_FUNC;
        symtab->symbols[index].ext.func.params =
            malloc(param_count * sizeof(char *));
        for (unsigned int i = 0; i < param_count; i++)
        {
            symtab->symbols[index].ext.func.params[i] = strdup(parameters[i]);
        }
        symtab->symbols[index].ext.func.params_count = param_count;
        symtab->symbols[index].ext.func.body = clone_ast(body);
        symtab->symbols[index].is_const = true;
        symtab->count++;
    }
    else
    {
        printf("Semantic Error: Name of variable too long!\n ");
        set_error_state_symtab(symtab);
    }
}

// grow the Symbol Table, return true if successfull, false if not
static bool grow_symtab(SymbolTable *symtab)
{
    if (symtab->count >= symtab->capacity)
    {
        // realloc with 0 is equal to free, what shouldn't happen here
        if (symtab->capacity == 0)
        {
            symtab->capacity = 8;
        }

        symtab->capacity *= 2;
        Symbol *new_symbol = (Symbol *)realloc(
            symtab->symbols, symtab->capacity * sizeof(Symbol));

        if (new_symbol == NULL)
        {
            printf("Fatal Error: Failed to allocate memory for symbol "
                   "table!\n");
            set_error_state_symtab(symtab);
            return false;
        }
        symtab->symbols = new_symbol;
    }
    return true;
}

// Check if a symbol is in the symbol table
static bool lookup_symbol(SymbolTable *symtab, const char *name)
{
    for (unsigned int i = 0; i < symtab->count; i++)
    {
        if (strcmp(name, symtab->symbols[i].name) == 0)
        {
            return true;
        }
    }
    // Try to find the variable in the parent scope
    if (symtab->enclosing_scope != NULL)
    {
        bool is_found = lookup_symbol(symtab->enclosing_scope, name);
        if (!is_found)
        {
            symtab->error_found = true;
        }
        return is_found;
    }

    printf("Semantic Error: Variable '%s' is not defined!\n", name);
    symtab->error_found = true;
    return false;
}

// Check if a function is in the symbol table
static bool lookup_function(SymbolTable *symtab, const char *name,
                            unsigned int arguments_count)
{
    for (unsigned int i = 0; i < symtab->count; i++)
    {
        if (strcmp(name, symtab->symbols[i].name) == 0)
        {
            Symbol function = symtab->symbols[i];

            if (function.symbol_kind != KIND_FUNC)
            {
                printf("Semantic Error: Function '%s' is not a function!\n",
                       name);
                symtab->error_found = true;
                return false;
            }

            if (function.ext.func.params_count != arguments_count)
            {
                printf("Semantic Error: Function '%s' expects %d arguments but "
                       "got %d!\n",
                       name, function.ext.func.params_count, arguments_count);
                symtab->error_found = true;
                return false;
            }
            return true;
        }
    }
    // Try to find the variable in the parent scope
    if (symtab->enclosing_scope != NULL)
    {
        bool is_found =
            lookup_function(symtab->enclosing_scope, name, arguments_count);
        if (!is_found)
        {
            symtab->error_found = true;
        }
        return is_found;
    }

    printf("Semantic Error: Function '%s' is not defined!\n", name);
    symtab->error_found = true;
    return false;
}

// Helper function to check if name is already defined in the buildin function
static bool is_buildin_function(const char *name)
{
    for (unsigned int i = 0; i < NUM_BUILTIN_FUNCTIONS; i++)
    {
        if (strcmp(BUILDIN_FUNCTIONS[i].name, name) == 0)
        {
            return true;
        }
    }
    return false;
}

// Init the symbol table
void init_symtab(SymbolTable *symtab)
{
    symtab->capacity = 8;
    symtab->symbols = malloc(symtab->capacity * sizeof(Symbol));
    symtab->count = 0;
    symtab->error_found = false;
    symtab->enclosing_scope = NULL;

    if (symtab->symbols == NULL)
    {
        printf("Fatal Error: Failed to allocate memory for symbol table!\n");
        symtab->error_found = true;
        return;
    }

    init_builtin_symbols(symtab);
}

// Add build in symbol like math constants to the symbol table
static void init_builtin_symbols(SymbolTable *symtab)
{
    for (unsigned int i = 0; i < NUM_BUILTINS; i++)
    {
        // Add the name, and lock it as a constant (true)
        define_symbol(symtab, BUILTIN_CONSTANTS[i].name, true);
    }
}

// Creates a child symbol scope linked to the parent scope. Used for function
// scopes -> returns Null if it couldn't be created
static SymbolTable *create_child_scope(SymbolTable *parent_scope,
                                       char **parameters,
                                       unsigned int parameter_count)
{
    SymbolTable *child_scope = malloc(sizeof(SymbolTable));
    init_symtab(child_scope);

    if (child_scope->error_found)
    {
        free_symtab(child_scope);
        free(child_scope);
        return NULL;
    }

    child_scope->enclosing_scope = parent_scope;

    // Define every varibale in the parameters
    for (unsigned int i = 0; i < parameter_count; i++)
    {
        define_symbol(child_scope, parameters[i], false);
        if (child_scope->error_found)
        {
            free_symtab(child_scope);
            free(child_scope);
            return NULL;
        }
    }
    return child_scope;
}

// Free the allocated memory of the symbol table
void free_symtab(SymbolTable *symtab)
{
    free(symtab->symbols);
    symtab->symbols = NULL;
    symtab->count = 0;
    symtab->capacity = 0;
    symtab->enclosing_scope = NULL;
}
/*
 * ####################
 * #    Interpreter   #
 * ####################
 */
// Rcursively walks the AST and calculates the result
Value evaluate(ASTNode *node, Interpreter *interpret, SymbolTable *symtab)
{
    // If an error was found return 0
    if (node == NULL || interpret->error_found)
    {
        return (Value){VAL_INT, {.i_val = 0}};
    }

    // Determine what kinde of node it is
    switch (node->type)
    {
        case NODE_LITERAL:
            return node->token.value;

        case NODE_BINOP:
        {
            return evaluate_binop(node, interpret, symtab);
        }

        case NODE_UNAOP:
        {
            return evaluate_unaop(node, interpret, symtab);
        }
        case NODE_CONST_ASSIGN:
        case NODE_ASSIGN:
        {
            Value left_val = evaluate(node->right, interpret, symtab);
            set_variable(interpret, node->left->token.name, left_val, false);
            return left_val;
        }

        case NODE_VAR:
        {
            return get_variable(interpret, node->token.name);
        }
        case NODE_IF:
        {
            Value left_val = evaluate(node->left, interpret, symtab);

            Value right_val = (Value){0};
            // Statement must be a boolean (no C style shinanigans)
            if (left_val.type == VAL_BOOL)
            {
                if (node->right != NULL && left_val.as.b_val == true)
                {
                    right_val = evaluate(node->right, interpret, symtab);
                }
                else if (node->ext.else_node != NULL &&
                         left_val.as.b_val == false)
                {
                    right_val =
                        evaluate(node->ext.else_node, interpret, symtab);
                }
                return right_val;
            }
            else
            {
                printf("Runtime Error: If-Statement requires a boolean "
                       "expression.\n");
                interpret->error_found = true;
                return (Value){VAL_INT, {.i_val = 0}};
            }
            return (Value){VAL_INT, {.i_val = 0}};
        }
        case NODE_COMPOUND:
        {
            Value left_val = evaluate(node->left, interpret, symtab);
            // Check if it needs to retun something, if yes break the cycle
            if (interpret->is_returning)
            {
                return left_val;
            }

            Value right_val = (Value){0};
            if (node->right != NULL)
            {
                right_val = evaluate(node->right, interpret, symtab);
                return right_val;
            }
            else
            {
                return left_val;
            }
            break;
        }
        case NODE_WHILE:
        {
            Value left_val = evaluate(node->left, interpret, symtab);
            Value right_val = (Value){0};
            // Statement must be a boolean (no C style shinanigans)
            if (left_val.type == VAL_BOOL)
            {
                while (left_val.as.b_val && !interpret->error_found)
                {
                    right_val = evaluate(node->right, interpret, symtab);
                    // Check if it needs to retun something, if yes break the
                    // loop
                    if (interpret->is_returning)
                    {
                        interpret->return_value = right_val;
                        break;
                    }
                    // Reevaluating the statement every time and check if
                    // its still a boolean
                    left_val = evaluate(node->left, interpret, symtab);
                    if (left_val.type != VAL_BOOL)
                    {
                        printf("Runtime Error: While-Statement requires a "
                               "boolean "
                               "expression.\n");
                        interpret->error_found = true;
                        break;
                    }
                }
                return right_val;
            }
            else
            {
                printf("Runtime Error: While-Statement requires a boolean "
                       "expression.\n");
                interpret->error_found = true;
                return (Value){VAL_INT, {.i_val = 0}};
            }

            return right_val;
        }
        case NODE_FUNC_DEF:
        { // Work already done in symnbol table -> return 0
            return (Value){VAL_INT, {.i_val = 0}};
            break;
        }
        case NODE_FUNC_CALL:
        {
            return evaluate_function_call(node, interpret, symtab);
        }
        case NODE_PRINT:
        {
            Value expr = evaluate(node->left, interpret, symtab);

            // Only print if evaluation didn't trigger a runtime error (like
            // divide by zero)
            if (!interpret->error_found)
            {
                if (expr.type == VAL_INT)
                {
                    printf("%lld\n", expr.as.i_val);
                }
                else if (expr.type == VAL_FLOAT)
                {
                    printf("%.7g\n", expr.as.f_val);
                }
                else if (expr.type == VAL_BOOL)
                {
                    printf("%s\n", expr.as.b_val ? "true" : "false");
                }
            }
            return (Value){VAL_INT, {.i_val = 0}};
        }
        case NODE_RETURN:
        {
            if (node->left != NULL)
            {
                Value expr = evaluate(node->left, interpret, symtab);
                interpret->return_value = expr;
                interpret->is_returning = true;
                return expr;
            }

            return (Value){VAL_INT, {.i_val = 0}};
        }
    }
    return (Value){VAL_INT, {.i_val = 0}};
}

// Evaluate binary operators
static Value evaluate_binop(ASTNode *node, Interpreter *interpret,
                            SymbolTable *symtab)
{
    Value left_val = evaluate(node->left, interpret, symtab);
    Value right_val = evaluate(node->right, interpret, symtab);

    if (interpret->error_found)
    {
        return (Value){VAL_INT, {.i_val = 0}};
    }

    Token op = node->token;
    if (left_val.type == VAL_BOOL && right_val.type == VAL_BOOL)
    {
        if (op.type == BIT_OR)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.b_val || right_val.as.b_val)}};
        }
        else if (op.type == BIT_XOR)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.b_val != right_val.as.b_val)}};
        }
        else if (op.type == BIT_AND)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.b_val && right_val.as.b_val)}};
        }
        else if (op.type == EQUAL)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.b_val == right_val.as.b_val)}};
        }
        else if (op.type == NOT_EQUAL)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.b_val != right_val.as.b_val)}};
        }
        else
        {
            printf("Runtime Error: Cannot perform arithmetic "
                   "operations on "
                   "booleans.\n");
            interpret->error_found = true;
            return (Value){VAL_INT, {.i_val = 0}};
        }
    }
    else if (left_val.type == VAL_BOOL || right_val.type == VAL_BOOL)
    {
        printf("Runtime Error: Cannot perform arithmetic "
               "operations on "
               "booleans.\n");
        interpret->error_found = true;
        return (Value){VAL_INT, {.i_val = 0}};
    }

    if (left_val.type == VAL_INT && right_val.type == VAL_INT)
    {
        Value result = {VAL_INT, {.i_val = 0}};
        if (op.type == PLUS)
        {
            if (SAFE_ADD(left_val.as.i_val, right_val.as.i_val,
                         &result.as.i_val))
            {
                printf("Runtime Error: Integer Overflow or "
                       "Underflow\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }

            return result;
        }
        else if (op.type == MINUS)
        {
            if (SAFE_SUB(left_val.as.i_val, right_val.as.i_val,
                         &result.as.i_val))
            {
                printf("Runtime Error: Integer Overflow or "
                       "Underflow\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }
            return result;
        }
        else if (op.type == MUL)
        {
            if (SAFE_MUL(left_val.as.i_val, right_val.as.i_val,
                         &result.as.i_val))
            {
                printf("Runtime Error: Integer Overflow or "
                       "Underflow\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }
            return result;
        }
        else if (op.type == DIV)
        {
            // The Division-by-Zero check returns!
            if (right_val.as.i_val == 0)
            {
                printf("Runtime Error: Division by zero\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }

            if (left_val.as.i_val == INT_MIN && right_val.as.i_val == -1)
            {
                printf("Runtime Error: Integer Overflow\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }
            double l_num = (double)left_val.as.i_val;
            double r_num = (double)right_val.as.i_val;
            return (Value){VAL_FLOAT, {.f_val = l_num / r_num}};
        }
        else if (op.type == BIT_OR)
        {
            return (Value){VAL_INT,
                           {.i_val = left_val.as.i_val | right_val.as.i_val}};
        }
        else if (op.type == BIT_XOR)
        {
            return (Value){VAL_INT,
                           {.i_val = left_val.as.i_val ^ right_val.as.i_val}};
        }
        else if (op.type == BIT_AND)
        {
            return (Value){VAL_INT,
                           {.i_val = left_val.as.i_val & right_val.as.i_val}};
        }
        else if (op.type == EQUAL)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.i_val == right_val.as.i_val)}};
        }
        else if (op.type == NOT_EQUAL)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.i_val != right_val.as.i_val)}};
        }
        else if (op.type == LESS)
        {
            return (Value){VAL_BOOL,
                           {.b_val = (left_val.as.i_val < right_val.as.i_val)}};
        }
        else if (op.type == EQUAL_LESS)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.i_val <= right_val.as.i_val)}};
        }
        else if (op.type == GREATER)
        {
            return (Value){VAL_BOOL,
                           {.b_val = (left_val.as.i_val > right_val.as.i_val)}};
        }
        else if (op.type == EQUAL_GREATER)
        {
            return (Value){
                VAL_BOOL, {.b_val = (left_val.as.i_val >= right_val.as.i_val)}};
        }
        else
        {
            printf("Runtime Error: Unknown operator\n");
            set_error_state_interpret(interpret);
            return (Value){VAL_INT, {.i_val = 0}};
        }
    }
    else
    {
        // If one is an INT, cast it to a double!
        double l_num = (left_val.type == VAL_FLOAT) ? left_val.as.f_val
                                                    : (double)left_val.as.i_val;
        double r_num = (right_val.type == VAL_FLOAT)
                           ? right_val.as.f_val
                           : (double)right_val.as.i_val;

        Value result = {VAL_FLOAT, {.f_val = 0.0}};

        if (node->token.type == PLUS)
        {
            result.as.f_val = l_num + r_num;
            return result;
        }
        else if (node->token.type == MINUS)
        {
            result.as.f_val = l_num - r_num;
            return result;
        }
        else if (node->token.type == MUL)
        {
            result.as.f_val = l_num * r_num;
            return result;
        }
        else if (node->token.type == DIV)
        {
            if (r_num == 0.0)
            {
                printf("Runtime Error: Division by zero\n");
                interpret->error_found = true;
                return (Value){VAL_INT, {.i_val = 0}};
            }
            result.as.f_val = l_num / r_num;
            return result;
        }
        else if (op.type == EQUAL)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num == r_num)}};
        }
        else if (op.type == NOT_EQUAL)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num != r_num)}};
        }
        else if (op.type == LESS)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num < r_num)}};
        }
        else if (op.type == EQUAL_LESS)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num <= r_num)}};
        }
        else if (op.type == GREATER)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num > r_num)}};
        }
        else if (op.type == EQUAL_GREATER)
        {
            return (Value){VAL_BOOL, {.b_val = (l_num >= r_num)}};
        }
        else
        {
            printf("Runtime Error: Unallowed operator\n");
            set_error_state_interpret(interpret);
            return (Value){VAL_INT, {.i_val = 0}};
        }
    }

    return (Value){VAL_INT, {.i_val = 0}};
}

// Evaluate unary operators
static Value evaluate_unaop(ASTNode *node, Interpreter *interpret,
                            SymbolTable *symtab)
{
    Value expr_val = evaluate(node->right, interpret, symtab);
    if (interpret->error_found)
        return (Value){VAL_INT, {.i_val = 0}};

    if (node->token.type == PLUS)
    {
        return expr_val;
    }
    if (expr_val.type == VAL_BOOL)
    {
        if (node->token.type == BIT_NOT)
        {
            return (Value){VAL_BOOL, {.b_val = !expr_val.as.b_val}};
        }
        else
        {
            printf("Runtime Error: Cannot perform arithmetic "
                   "operations on "
                   "booleans.\n");
            interpret->error_found = true;
            return (Value){VAL_INT, {.i_val = 0}};
        }
    }
    else if (node->token.type == MINUS)
    {
        if (expr_val.type == VAL_INT)
        {

            if (expr_val.as.i_val == (INT_MIN))
            {
                printf("Runtime Error: Integer Overflow\n");
                set_error_state_interpret(interpret);
                return (Value){VAL_INT, {.i_val = 0}};
            }
            return (Value){VAL_INT, {.i_val = -expr_val.as.i_val}};
        }
        else if (expr_val.type == VAL_FLOAT)
        {
            return (Value){VAL_FLOAT, {.f_val = -expr_val.as.f_val}};
        }
    }
    else if (node->token.type == BIT_NOT)
    {
        if (expr_val.type == VAL_INT)
        {
            return (Value){VAL_INT, {.i_val = ~expr_val.as.i_val}};
        }
        else
        {
            printf("Runtime Error: Cannot invert decimal number "
                   "'%f'\n",
                   expr_val.as.f_val);
            set_error_state_interpret(interpret);
            return (Value){VAL_INT, {.i_val = 0}};
        }
    }
    return (Value){VAL_INT, {.i_val = 0}};
}

// Evaluate function calls
static Value evaluate_function_call(ASTNode *node, Interpreter *interpret,
                                    SymbolTable *symtab)
{
    // Check if its a buildin function
    for (unsigned int i = 0; i < NUM_BUILTIN_FUNCTIONS; i++)
    {
        if (strcmp(BUILDIN_FUNCTIONS[i].name, node->ext.func_call.name) == 0)
        {
            Value *arguments =
                malloc(node->ext.func_call.arg_count * sizeof(Value));
            for (unsigned int j = 0; j < node->ext.func_call.arg_count; j++)
            {
                arguments[j] =
                    evaluate(node->ext.func_call.args[j], interpret, symtab);
            }

            if (interpret->error_found)
            {
                free(arguments);
                return (Value){.type = VAL_INT, .as = {.i_val = 0}};
            }
            Value result = BUILDIN_FUNCTIONS[i].fn(
                interpret, arguments, node->ext.func_call.arg_count);
            free(arguments);
            return result;
        }
    }

    Symbol *function = get_function(symtab, node->ext.func_call.name);
    // Should be prevented, but check is still good
    if (function == NULL)
    {
        set_error_state_interpret(interpret);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    RuntimeScope *child_scope = init_scope(interpret->current_scope);

    if (child_scope == NULL)
    {
        set_error_state_interpret(interpret);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    // Set the child scope as current scope, so it can be used in the
    // interpreter
    interpret->current_scope = child_scope;
    // Set all arguments as variables  in the scope
    for (unsigned int i = 0; i < node->ext.func_call.arg_count; i++)
    {
        Value arg_val =
            evaluate(node->ext.func_call.args[i], interpret, symtab);
        set_variable(interpret, function->ext.func.params[i], arg_val, true);
    }
    if (interpret->error_found)
    {
        interpret->current_scope = child_scope->enclosing_scope;
        free_scope(child_scope);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    Value result = evaluate(function->ext.func.body, interpret, symtab);
    // Evaluate the body of the function
    if (interpret->error_found)
    {
        interpret->current_scope = child_scope->enclosing_scope;
        free_scope(child_scope);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    // Restore the scope
    interpret->current_scope = child_scope->enclosing_scope;
    free_scope(child_scope);
    if (interpret->is_returning)
    {
        interpret->is_returning = false;
        return interpret->return_value;
    }
    else
    {
        return result;
    }
}

// Initialize the interpreter struct
void init_interpreter(Interpreter *interpret)
{
    interpret->buffer = NULL;
    interpret->length = 0;
    interpret->position = 0;
    interpret->current_token = (Token){0};
    interpret->error_found = false;
    interpret->is_returning = false;
    interpret->return_value = (Value){VAL_INT, {.i_val = 0}};

    // Init the scope
    interpret->current_scope = init_scope(NULL);

    // Check current_scope first — dereferencing NULL pointer is undefined
    // behavior
    if (interpret->current_scope == NULL ||
        interpret->current_scope->memory == NULL)
    {
        interpret->error_found = true;
        return;
    }
    set_math_const(interpret);
}

// Initialize a scope, if its the top one set parent_scope to NULL
static RuntimeScope *init_scope(RuntimeScope *parent_scope)
{
    RuntimeScope *new_scope = malloc(sizeof(RuntimeScope));
    if (new_scope == NULL)
    {
        printf("Fatal Error: Failed to allocate memory for scopes!\n");
        return NULL;
    }
    new_scope->enclosing_scope = parent_scope;
    new_scope->memory_count = 0;
    new_scope->memory_capacity = 8;

    new_scope->memory = malloc(new_scope->memory_capacity * sizeof(MemorySlot));

    if (new_scope->memory == NULL)
    {
        free_scope(new_scope);
        printf("Fatal Error: Failed to allocate memory for scopes!\n");
        return NULL;
    }
    return new_scope;
}

// Free scope memory
static void free_scope(RuntimeScope *scope)
{
    free(scope->memory);
    free(scope);
}

// Set mathematical constants in the interpreter.
static void set_math_const(Interpreter *interpret)
{
    for (unsigned int i = 0; i < NUM_BUILTINS; i++)
    {
        // Inject the actual value into RAM
        set_variable(interpret, BUILTIN_CONSTANTS[i].name,
                     BUILTIN_CONSTANTS[i].value, false);
    }
}

// Resets the parser for a brand new line of text
void reset_interpreter_line(Interpreter *interpret, char *buffer)
{
    interpret->buffer = buffer;
    interpret->length = strlen(buffer);
    interpret->position = 0; // Reset the reading cursor to the start!
    interpret->current_token = (Token){0}; // Wipe the old token
    interpret->error_found = false;        // Forgive any past syntax errors!
}

// Free the allocated memory of the variable structur in the interpreter
void free_interpreter(Interpreter *interpret)
{
    free_scope(interpret->current_scope);
}

// Check if a variable name is already known and if not save it as a new
// name
static void set_variable(Interpreter *interpret, const char *name, Value value,
                         bool is_local)
{
    // Check if the variable already exist in the scope or parent scope and
    // if yes update it
    RuntimeScope *scope = interpret->current_scope;
    while (scope != NULL)
    {
        for (unsigned int i = 0; i < scope->memory_count; i++)
        {
            if (strcmp(scope->memory[i].name, name) == 0)
            {
                scope->memory[i].value = value;
                return;
            }
        }
        if (is_local)
        {
            break;
        }
        scope = (RuntimeScope *)scope->enclosing_scope;
    }

    if (interpret->current_scope->memory_count >=
        interpret->current_scope->memory_capacity)
    {
        // realloc with 0 is equal to free, what shouldn't happen here
        if (interpret->current_scope->memory_capacity == 0)
        {
            interpret->current_scope->memory_capacity = 8;
        }

        interpret->current_scope->memory_capacity *= 2;
        MemorySlot *new_memory = (MemorySlot *)realloc(
            interpret->current_scope->memory,
            interpret->current_scope->memory_capacity * sizeof(MemorySlot));

        if (new_memory == NULL)
        {
            printf("Fatal Error: Failed to allocate memory for variables!\n");
            set_error_state_interpret(interpret);
            return;
        }
        interpret->current_scope->memory = new_memory;
    }
    unsigned int index = interpret->current_scope->memory_count;
    if (strlen(name) < NAME_LENGTH)
    {
        strcpy(interpret->current_scope->memory[index].name, name);
        interpret->current_scope->memory[index].value = value;
        interpret->current_scope->memory_count++;
    }
    else
    {
        printf("Runtime Error: Name of variable too long!\n ");
        set_error_state_interpret(interpret);
    }
}

// Check if a variable exists in the symbol table and if yes returns the
// value
static Value get_variable(Interpreter *interpret, const char *name)
{
    // Check if the variable already exist in the scope or parent scope and
    // if yes update it
    RuntimeScope *scope = interpret->current_scope;
    while (scope != NULL)
    {
        for (unsigned int i = 0; i < scope->memory_count; i++)
        {
            if (strcmp(scope->memory[i].name, name) == 0)
            {
                return scope->memory[i].value;
            }
        }
        scope = (RuntimeScope *)scope->enclosing_scope;
    }

    printf("Runtime Error: Variable '%s' is not defined!\n", name);
    set_error_state_interpret(interpret);
    return (Value){VAL_INT, {.i_val = 0}};
}

// Get the functgion from the symbol table
static Symbol *get_function(SymbolTable *symtab, const char *name)
{
    Symbol *function = NULL;
    for (unsigned int i = 0; i < symtab->count; i++)
    {
        if (strcmp(symtab->symbols[i].name, name) == 0)
        {
            // Semantic analyzer should caught every error befor
            function = &symtab->symbols[i];
        }
    }
    return function;
}

/*
 * ####################
 * # Helper Functions #
 * ####################
 */
// Helper functiion for adding single characters to a token
static void make_simple_token(Interpreter *interpret, TokenType type)
{
    Token token = {0};
    token.type = type;
    token.value = (Value){VAL_INT, {.i_val = 0}};

    // Save to the interpreter state
    interpret->current_token = token;

    // Do the annoying position increment here, once!
    interpret->position++;
}

// Helper function to set the error state of the interpreter
static void set_error_state_interpret(Interpreter *interpret)
{
    interpret->error_found = true;
    interpret->current_token.type = ERROR;
    interpret->current_token.value = (Value){VAL_INT, {.i_val = 0}};
}

// Helper function to set the error state of the symbol table
static void set_error_state_symtab(SymbolTable *symtab)
{
    symtab->error_found = true;
}

// Check if its a plus or minus operator
static bool is_additive_op(TokenType type)
{
    return (bool)(type == PLUS || type == MINUS);
}

// Check if its a multiplication or dividing operator
static bool is_multiplicative_op(TokenType type)
{
    return (bool)(type == MUL || type == DIV);
}

// Check if its an assign operation
static bool is_assign_op(TokenType type) { return (bool)(type == ASSIGN); }

// Check if its a relation type
static bool is_relation_op(TokenType type)
{
    return (bool)((type == EQUAL_GREATER) || (type == EQUAL_LESS) ||
                  (type == LESS) || (type == GREATER));
}

// Get the next character without increasing the position of the interpreter
static char peek(Interpreter *interpret)
{
    unsigned int position = interpret->position + 1;
    // Make sure there is no overflow while reading the buffer
    if (position >= strlen(interpret->buffer))
    {
        return '\0';
    }
    if (position <= interpret->length)
    {
        return interpret->buffer[position];
    }
    return '\0';
}

// Helper to build the statement compound
static ASTNode *parse_compound(Interpreter *interpret)
{
    if (interpret->current_token.type == EOL)
    {
        get_next_token(interpret);
    }
    if (interpret->current_token.type == RBRACE)
    {
        return NULL;
    }
    ASTNode *stmt = statement(interpret);
    ASTNode *next = parse_compound(interpret);
    return create_compound_node(stmt, next);
}

// Skip EOL char
static void skip_eol(Interpreter *interpret)
{
    while (interpret->current_token.type == EOL)
    {
        get_next_token(interpret);
    }
}

// Wrapper function to use c build in trigonomy function
static Value buildin_unary_math(char *func_name, Interpreter *interpret,
                                Value *arguments, unsigned int arguments_count,
                                double (*trigonomy_function)(double))
{
    if (arguments_count != 1)
    {
        printf("Runtime Error: Builtin %s takes 1 argument, got %d!\n",
               func_name, arguments_count);
        set_error_state_interpret(interpret);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    if (arguments[0].type == VAL_BOOL)
    {
        printf("Runtime Error: Builtin %s takes integeger argument, got "
               "boolean!\n",
               func_name);
        set_error_state_interpret(interpret);
        return (Value){VAL_INT, {.i_val = 0}};
    }

    double input = 0.0;
    if (arguments[0].type == VAL_INT)
    {
        input = (double)arguments[0].as.i_val;
    }
    else
    {
        input = arguments[0].as.f_val;
    }

    return (Value){VAL_FLOAT, {.f_val = trigonomy_function(input)}};
}

// Wrapper function to use c build in sin function
static Value buildin_sin(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count)
{
    return buildin_unary_math("sin", interpret, arguments, arguments_count,
                              sin);
}

// Wrapper function to use c build in asin function
static Value buildin_asin(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count)
{
    return buildin_unary_math("asin", interpret, arguments, arguments_count,
                              asin);
}

// Wrapper function to use c build in cos function
static Value buildin_cos(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count)
{
    return buildin_unary_math("cos", interpret, arguments, arguments_count,
                              cos);
}

// Wrapper function to use c build in acos function
static Value buildin_acos(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count)
{
    return buildin_unary_math("acos", interpret, arguments, arguments_count,
                              acos);
}
// Wrapper function to use c build in tan function
static Value buildin_tan(Interpreter *interpret, Value *arguments,
                         unsigned int arguments_count)
{
    return buildin_unary_math("tan", interpret, arguments, arguments_count,
                              tan);
}

// Wrapper function to use c build in atan function
static Value buildin_atan(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count)
{
    return buildin_unary_math("atan", interpret, arguments, arguments_count,
                              atan);
}

// Wrapper function to use c build in sqrt function
static Value buildin_sqrt(Interpreter *interpret, Value *arguments,
                          unsigned int arguments_count)
{
    return buildin_unary_math("sqrt", interpret, arguments, arguments_count,
                              sqrt);
}

// Wrapper function to use c build in floor function
static Value buildin_floor(Interpreter *interpret, Value *arguments,
                           unsigned int arguments_count)
{
    return buildin_unary_math("floor", interpret, arguments, arguments_count,
                              floor);
}
