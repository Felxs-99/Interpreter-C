#include "interpreter.h"
#include "overflow.h"
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Parser function prototypes
static ASTNode *create_num_node(Token token);
static ASTNode *create_binop_node(ASTNode *left, Token op, ASTNode *right);
static ASTNode *create_unaop_node(Token op, ASTNode *right);
static ASTNode *create_assign_node(ASTNode *right, Token op, ASTNode *left);
static ASTNode *create_var_node(Token id);
static bool eat(token_types token, Interpreter *interprete);
static ASTNode *term(Interpreter *interpret);
static ASTNode *factor(Interpreter *interpret);

// Helper function prototypes
static uint8_t convert_char(char character);
static bool multiple_digit_number(int *number, int digit_to_add);
static void make_single_char_token(Interpreter *interpret, token_types type,
                                   int value);
static void set_error_state(Interpreter *interpret);
static bool is_additive_op(token_types type);
static bool is_multiplicative_op(token_types type);
static bool is_assign_op(token_types type);
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
static ASTNode *create_assign_node(ASTNode *right, Token op, ASTNode *left)
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
ASTNode *expr(Interpreter *interpret)
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
    else if (token.type)
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
    if (isdigit(current_char))
    {
        token.type = INT;
        token.value = convert_char(current_char);
        interpret->position++;

        // Get every following digit and make it one number
        while (isdigit(interpret->buffer[interpret->position]))
        {
            if (!multiple_digit_number(
                    &token.value,
                    convert_char(interpret->buffer[interpret->position])))
            {
                printf("Syntax Error: The number starting with '%d...' is too "
                       "large!\n",
                       token.value);
                set_error_state(interpret);
                return;
            }
            interpret->position++;
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
        make_single_char_token(interpret, PLUS, 0);
        return;
    case '-':
        make_single_char_token(interpret, MINUS, 0);
        return;
    case '*':
        make_single_char_token(interpret, MUL, 0);
        return;
    case '/':
        make_single_char_token(interpret, DIV, 0);
        return;
    case '(':
        make_single_char_token(interpret, LPAREN, 0);
        return;
    case ')':
        make_single_char_token(interpret, RPAREN, 0);
        return;
    case '=':
        make_single_char_token(interpret, ASSIGN, 0);
        return;
    case '\n':
    case '\0':
        make_single_char_token(interpret, EOL, 0);
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
int evaluate(ASTNode *node, Interpreter *interpret)
{
    // If an error was found return 0
    if (node == NULL || interpret->error_found)
    {
        return 0;
    }

    // Determine what kinde of node it is
    switch (node->type)
    {
    case NODE_NUM:
        return node->token.value;

    case NODE_BINOP:
    {
        int left_val = evaluate(node->left, interpret);
        int right_val = evaluate(node->right, interpret);
        int result = 0;

        if (interpret->error_found)
            return 0;
        if (node->token.type == PLUS)
        {
            if (SAFE_ADD(left_val, right_val, &result))
            {
                printf("Runtime Error: Integer Overflow or Underflow\n");
                set_error_state(interpret);
                return 0;
            }

            return result;
        }
        else if (node->token.type == MINUS)
        {
            if (SAFE_SUB(left_val, right_val, &result))
            {
                printf("Runtime Error: Integer Overflow or Underflow\n");
                set_error_state(interpret);
                return 0;
            }
            return result;
        }
        else if (node->token.type == MUL)
        {
            if (SAFE_MUL(left_val, right_val, &result))
            {
                printf("Runtime Error: Integer Overflow or Underflow\n");
                set_error_state(interpret);
                return 0;
            }
            return result;
        }
        else if (node->token.type == DIV)
        {
            // The Division-by-Zero check returns!
            if (right_val == 0)
            {
                printf("Runtime Error: Division by zero\n");
                set_error_state(interpret);
                return 0;
            }

            if (left_val == INT_MIN && right_val == -1)
            {
                printf("Runtime Error: Integer Overflow\n");
                set_error_state(interpret);
                return 0;
            }
            return left_val / right_val;
        }
        break;
    }

    case NODE_UNAOP:
    {
        int expr_val = evaluate(node->right, interpret);
        if (interpret->error_found)
            return 0;

        if (node->token.type == PLUS)
        {
            return +expr_val;
        }
        else if (node->token.type == MINUS)
        {
            if (expr_val == (INT_MIN))
            {
                printf("Runtime Error: Integer Overflow\n");
                set_error_state(interpret);
                return 0;
            }
            return -expr_val;
        }
    }
        return 0;
    }
}
/*
 * ####################
 * # Helper Functions #
 * ####################
 */
// Helper functiion for adding single characters to a token
static void make_single_char_token(Interpreter *interpret, token_types type,
                                   int value)
{
    Token token = {0};
    token.type = type;
    token.value = value;

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
    interpret->current_token.value = 0;
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
static uint8_t convert_char(char character)
{
    return (uint8_t)(character - '0');
}

// Check if the next lower digit can be added and do it, if its possible
static bool multiple_digit_number(int *number, int digit_to_add)
{
    int result = *number;
    // Check if it can be shiftet to the left (base 10 shift)
    if (SAFE_MUL(*number, 10, &result))
    {
        return false;
    }
    // Check if the digit can be added
    if (SAFE_ADD(result, digit_to_add, &result))
    {
        return false;
    }
    *number = result;
    return true;
}

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
}
