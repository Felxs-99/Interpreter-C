#include "interpreter.h"
#include "overflow.h"
#include <ctype.h>
#include <iso646.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parser function prototypes
static ASTNode *create_num_node(Token token);
static ASTNode *create_binop_node(ASTNode *left, Token op, ASTNode *right);
static ASTNode *create_unaop_node(Token op, ASTNode *right);
static ASTNode *create_assign_node(ASTNode *left, Token op, ASTNode *right);
static ASTNode *create_var_node(Token id);
static bool eat(token_types token, Interpreter *interprete);
static ASTNode *expr(Interpreter *interpret);
static ASTNode *term(Interpreter *interpret);
static ASTNode *factor(Interpreter *interpret);

// Interpreter function prototypes
static void set_variable(Interpreter *interpret, const char *name, Value value);
static Value get_variable(Interpreter *interpret, const char *name);
void set_math_const(Interpreter *interpret);

// Helper function prototypes
static void make_single_char_token(Interpreter *interpret, token_types type);
static void set_error_state(Interpreter *interpret);
static bool is_additive_op(token_types type);
static bool is_multiplicative_op(token_types type);
static bool is_assign_op(token_types type);
static void set_constant(Interpreter *interpret, const char *name, Value value);

/*
 * ####################
 * #     PARSER       #
 * ####################
 */

// Ast node for numbers
static ASTNode *create_num_node(Token token)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = NODE_NUM;
    node->token = token;
    node->left = NULL;
    node->right = NULL;
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
    return node;
}

// Clean up for ast
void free_ast(ASTNode *node)
{
    if (node == NULL)
        return;
    free_ast(node->left); // Free children first (Post-order traversal)
    free_ast(node->right);
    free(node); // Then free the parent
}

// Eat the provided token
static bool eat(token_types token, Interpreter *interpret)
{
    if (interpret->current_token.type == token)
    {
        get_next_token(interpret);
        return true;
    }
    return false;
}

ASTNode *statement(Interpreter *interpret)
{
    ASTNode *left_node = expr(interpret);
    Token token = {};

    // Stop evaluating if a syntax error was found in expr()
    if (interpret->error_found)
    {
        free_ast(left_node);
        return NULL;
    }

    if (is_assign_op(interpret->current_token.type))
    {
        if (left_node->type != NODE_VAR)
        {
            printf("Syntax Error: You can only assign values to variables.\n");
            set_error_state(interpret);
            free_ast(left_node);
            return NULL;
        }
        token = interpret->current_token;
        if (!eat(ASSIGN, interpret))
        {
            printf("Syntax Error: Only values can be assigned to variables\n");
            set_error_state(interpret);
            free_ast(left_node);
            return NULL;
        }

        // Grab the right side as a node
        ASTNode *right_node = expr(interpret);

        return create_assign_node(left_node, token, right_node);
    }

    // No assigment was used, just return the node from expr
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

        // Grab the right side as a node
        ASTNode *right_node = term(interpret);

        // Stitch them together instead of doing math
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

        // Grab the right side as a node
        ASTNode *right_node = factor(interpret);

        // Stitch them together instead of doing math
        left_node = create_binop_node(left_node, token, right_node);
    }
    return left_node;
}

// factor : (PLUS | MINUS) factor | INTEGER | LPAREN expr RPAREN
static ASTNode *factor(Interpreter *interpret)
{
    Token token = interpret->current_token;
    // For unary operators
    if ((token.type == PLUS) || (token.type == MINUS))
    {
        eat(token.type, interpret);
        ASTNode *result = factor(interpret);
        return create_unaop_node(token, result);
    }
    // For integer values
    else if (token.type == INT)
    {
        eat(INT, interpret);
        return create_num_node(token);
    }
    // For decimal values
    else if (token.type == FLOAT)
    {
        eat(FLOAT, interpret);
        return create_num_node(token);
    }
    // For parentheses
    else if (token.type == LPAREN)
    {
        eat(LPAREN, interpret);
        ASTNode *result = expr(interpret);
        if (!eat(RPAREN, interpret))
        {
            printf("Syntax Error: Missing closing ')'\n");
            set_error_state(interpret);
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
        return create_var_node(token);
    }
    else
    {
        printf("Syntax Error: Expected an Integer, or '('\n");
        set_error_state(interpret);
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
        // Get every following digit and make it one number
        while (isdigit(interpret->buffer[interpret->position]) ||
               interpret->buffer[interpret->position] == '.')
        {
            char c = interpret->buffer[interpret->position];

            // Check for decimal point
            if (c == '.')
            {
                if (has_decimal)
                {
                    printf("Syntax Error: Multiple decimal points!\n");
                    set_error_state(interpret);
                    return;
                }

                has_decimal = true;
            }
            // Prevent temp buffer overflow
            if (temp_pos >= 64)
            {
                printf("Value Error: Given Number is too long!\n");
                set_error_state(interpret);
                return;
            }

            temp_num[temp_pos++] = c;

            interpret->position++;
        }

        temp_num[temp_pos] = '\0';

        if (has_decimal)
        {
            token.type = FLOAT;
            token.value.type = VAL_FLOAT;
            token.value.as.f_val = strtod(temp_num, NULL);
        }
        else
        {
            token.type = INT;
            token.value.type = VAL_INT;
            token.value.as.i_val = atoi(temp_num);
        }

        interpret->current_token = token;
        return;
    }

    // Check if its a letter
    if (isalpha(current_char))
    {
        token.type = ID;
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
                set_error_state(interpret);
                return;
            }

            token.name[position] = interpret->buffer[interpret->position];
            interpret->position++;
        }

        // Add traling \0
        token.name[++position] = '\0';
        interpret->current_token = token;
        return;
    }

    // Check for known symbols and create the correspondig token
    switch (current_char)
    {
        case '+':
            make_single_char_token(interpret, PLUS);
            return;
        case '-':
            make_single_char_token(interpret, MINUS);
            return;
        case '*':
            make_single_char_token(interpret, MUL);
            return;
        case '/':
            make_single_char_token(interpret, DIV);
            return;
        case '(':
            make_single_char_token(interpret, LPAREN);
            return;
        case ')':
            make_single_char_token(interpret, RPAREN);
            return;
        case '=':
            make_single_char_token(interpret, ASSIGN);
            return;
        case '\n':
        case '\0':
            make_single_char_token(interpret, EOL);
            return;
        default:
            break;
    }

    // Unknow character
    printf("Syntax Error: Unknown character '%c'\n", current_char);
    set_error_state(interpret);
    return;
}

/*
 * ####################
 * #    Interpreter   #
 * ####################
 */
// Rcursively walks the AST and calculates the result
Value evaluate(ASTNode *node, Interpreter *interpret)
{
    // If an error was found return 0
    if (node == NULL || interpret->error_found)
    {
        return (Value){VAL_INT, {.i_val = 0}};
    }

    // Determine what kinde of node it is
    switch (node->type)
    {
        case NODE_NUM:
            return node->token.value;

        case NODE_BINOP:
        {
            Value left_val = evaluate(node->left, interpret);
            Value right_val = evaluate(node->right, interpret);

            if (interpret->error_found)
                return (Value){VAL_INT, {.i_val = 0}};

            if (left_val.type == VAL_INT && right_val.type == VAL_INT)
            {
                Value result = {VAL_INT, {.i_val = 0}};
                if (node->token.type == PLUS)
                {
                    if (SAFE_ADD(left_val.as.i_val, right_val.as.i_val,
                                 &result.as.i_val))
                    {
                        printf(
                            "Runtime Error: Integer Overflow or Underflow\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }

                    return result;
                }
                else if (node->token.type == MINUS)
                {
                    if (SAFE_SUB(left_val.as.i_val, right_val.as.i_val,
                                 &result.as.i_val))
                    {
                        printf(
                            "Runtime Error: Integer Overflow or Underflow\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }
                    return result;
                }
                else if (node->token.type == MUL)
                {
                    if (SAFE_MUL(left_val.as.i_val, right_val.as.i_val,
                                 &result.as.i_val))
                    {
                        printf(
                            "Runtime Error: Integer Overflow or Underflow\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }
                    return result;
                }
                else if (node->token.type == DIV)
                {
                    // The Division-by-Zero check returns!
                    if (right_val.as.i_val == 0)
                    {
                        printf("Runtime Error: Division by zero\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }

                    if (left_val.as.i_val == INT_MIN &&
                        right_val.as.i_val == -1)
                    {
                        printf("Runtime Error: Integer Overflow\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }
                    double l_num = (double)left_val.as.i_val;
                    double r_num = (double)right_val.as.i_val;
                    return (Value){VAL_FLOAT, {.f_val = l_num / r_num}};
                }
            }
            else
            {
                // If one is an INT, cast it to a double!
                double l_num = (left_val.type == VAL_FLOAT)
                                   ? left_val.as.f_val
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
            }

            return (Value){VAL_INT, {.i_val = 0}};
        }

        case NODE_UNAOP:
        {
            Value expr_val = evaluate(node->right, interpret);
            if (interpret->error_found)
                return (Value){VAL_INT, {.i_val = 0}};

            if (node->token.type == PLUS)
            {
                return expr_val;
            }
            else if (node->token.type == MINUS)
            {
                if (expr_val.type == VAL_INT)
                {

                    if (expr_val.as.i_val == (INT_MIN))
                    {
                        printf("Runtime Error: Integer Overflow\n");
                        set_error_state(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }
                    return (Value){VAL_INT, {.i_val = -expr_val.as.i_val}};
                }
                else if (expr_val.type == VAL_FLOAT)
                {
                    return (Value){VAL_FLOAT, {.f_val = -expr_val.as.f_val}};
                }
            }
            return (Value){VAL_INT, {.i_val = 0}};
        }

        case NODE_ASSIGN:
        {
            Value left_val = evaluate(node->right, interpret);
            set_variable(interpret, node->left->token.name, left_val);
            return left_val;
        }

        case NODE_VAR:
        {
            return get_variable(interpret, node->token.name);
        }
    }
    return (Value){VAL_INT, {.i_val = 0}};
}

// Initialize the interpreter struct
void init_interpreter(Interpreter *interpret)
{
    interpret->buffer = NULL;
    interpret->length = 0;
    interpret->position = 0;
    interpret->current_token = (Token){0};
    interpret->error_found = false;

    interpret->var_count = 0;
    interpret->var_capacity = 8;

    interpret->variables = malloc(interpret->var_capacity * sizeof(Variable));

    if (interpret->variables == NULL)
    {
        printf("Fatal Error: Failed to allocate memory for variables!\n");
        interpret->error_found = true;
        return;
    }
    set_math_const(interpret);
}

// Set mathematical constants in the interpreter.
void set_math_const(Interpreter *interpret)
{
    // Eulers number
    set_constant(interpret, "e", (Value){VAL_FLOAT, {.f_val = 2.718282}});
    // Pi
    set_constant(interpret, "pi", (Value){VAL_FLOAT, {.f_val = 3.141593}});
    // Speed of light in m/s
    set_constant(interpret, "c", (Value){VAL_INT, {.i_val = 299792458}});
    // Vacuumn permittivity in (A*s)/(V*m)
    set_constant(interpret, "ep", (Value){VAL_FLOAT, {.f_val = 8.854188}});
    // Vacuumn permeability in N/(A²)
    set_constant(interpret, "mu", (Value){VAL_FLOAT, {.f_val = 1.256637}});
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
    free(interpret->variables);
    interpret->variables = NULL;
    interpret->var_count = 0;
    interpret->var_capacity = 0;
}

// Check if a variable name is already known and if not save it as a new name
static void set_variable(Interpreter *interpret, const char *name, Value value)
{
    // Check if the variable already exist and if yes update it
    for (unsigned int i = 0; i < interpret->var_count; i++)
    {
        if (strcmp(interpret->variables[i].name, name) == 0)
        {
            if (interpret->variables[i].is_const)
            {
                printf("Runtime Error: %s is a constant!\n",
                       interpret->variables[i].name);
                set_error_state(interpret);
                return;
            }
            interpret->variables[i].value = value;
            return;
        }
    }

    if (interpret->var_count >= interpret->var_capacity)
    {
        interpret->var_capacity *= 2;
        Variable *new_memory = (Variable *)realloc(
            interpret->variables, interpret->var_capacity * sizeof(Variable));

        if (new_memory == NULL)
        {
            printf("Fatal Error: Failed to allocate memory for variables!\n");
            set_error_state(interpret);
            return;
        }
        interpret->variables = new_memory;
    }
    unsigned int index = interpret->var_count;
    strcpy(interpret->variables[index].name, name);
    interpret->variables[index].value = value;
    interpret->variables[index].is_const = false;
    interpret->var_count++;
}

// Check if a variable exists in the symbol table and if yes returns the value
static Value get_variable(Interpreter *interpret, const char *name)
{
    for (unsigned int i = 0; i < interpret->var_count; i++)
    {
        if (strcmp(interpret->variables[i].name, name) == 0)
        {
            return interpret->variables[i].value;
        }
    }

    printf("Runtime Error: Variable '%s' is not defined!\n", name);
    set_error_state(interpret);
    return (Value){VAL_INT, {.i_val = 0}};
}

/*
 * ####################
 * # Helper Functions #
 * ####################
 */
// Helper functiion for adding single characters to a token
static void make_single_char_token(Interpreter *interpret, token_types type)
{
    Token token = {0};
    token.type = type;
    token.value = (Value){VAL_INT, {.i_val = 0}};

    // Save to the interpreter state
    interpret->current_token = token;

    // Do the annoying position increment here, once!
    interpret->position++;
}

// Helper function to set the error state
static void set_error_state(Interpreter *interpret)
{
    interpret->error_found = true;
    interpret->current_token.type = ERROR;
    interpret->current_token.value = (Value){VAL_INT, {.i_val = 0}};
}

// Check if its a plus or minus operator
static bool is_additive_op(token_types type)
{
    return (bool)(type == PLUS || type == MINUS);
}

// Check if its a multiplication or dividing operator
static bool is_multiplicative_op(token_types type)
{
    return (bool)(type == MUL || type == DIV);
}

// Check if its an assign operation
static bool is_assign_op(token_types type) { return (bool)(type == ASSIGN); }
// Converts a single digit into a uint8_t number

// Safely injects a locked constant into the memory bank
static void set_constant(Interpreter *interpret, const char *name, Value value)
{
    set_variable(interpret, name, value);
    int index = interpret->var_count - 1;
    interpret->variables[index].is_const = true;
}
