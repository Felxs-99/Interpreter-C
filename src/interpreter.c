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
static bool eat(TokenTypes token, Interpreter *interprete);
static ASTNode *bitwise_or_expr(Interpreter *interpret);
static ASTNode *bitwise_xor_expr(Interpreter *interpret);
static ASTNode *bitwise_and_expr(Interpreter *interpret);
static ASTNode *expr(Interpreter *interpret);
static ASTNode *term(Interpreter *interpret);
static ASTNode *factor(Interpreter *interpret);

// Symbol table function prototypes
static bool lookup_symbol(SymbolTable *symtab, const char *name);
static void define_symbol(SymbolTable *symtab, const char *name, bool is_const);

// Interpreter function prototypes
static void set_variable(Interpreter *interpret, const char *name, Value value);
static Value get_variable(Interpreter *interpret, const char *name);
void set_math_const(Interpreter *interpret);

// Helper function prototypes
static void make_single_char_token(Interpreter *interpret, TokenTypes type);
static void set_error_state_interpret(Interpreter *interpret);
// Helper function to set the error state of the interpreter
static void set_error_state_symtab(SymbolTable *symtab);
static bool is_additive_op(TokenTypes type);
static bool is_multiplicative_op(TokenTypes type);
static bool is_assign_op(TokenTypes type);
static void set_constant(Interpreter *interpret, const char *name, Value value);
static char peek(Interpreter *interpret);

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
static bool eat(TokenTypes token, Interpreter *interpret)
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
    ASTNode *left_node = bitwise_or_expr(interpret);
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
            set_error_state_interpret(interpret);
            free_ast(left_node);
            return NULL;
        }
        token = interpret->current_token;
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
            free(left_node);
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
            free(left_node);
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
    ASTNode *left_node = expr(interpret);
    Token token = {0};

    while (interpret->current_token.type == BIT_AND)
    {
        if (interpret->error_found)
        {
            free(left_node);
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
        return create_var_node(token);
    }
    else
    {
        printf("Syntax Error: Expected an Integer, an unary operator or '('\n");
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
                    temp_num[temp_pos++] =
                        interpret->buffer[interpret->position++];
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

                token.type = INT;
                token.value.type = VAL_INT;
                token.value.as.i_val = strtol(temp_num, NULL, 2);

                interpret->current_token = token;
                return;
            }
            else if (next_char == 'x')
            {
                interpret->position += 2;

                while (isxdigit(interpret->buffer[interpret->position]))
                {
                    temp_num[temp_pos++] =
                        interpret->buffer[interpret->position++];
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

                token.type = INT;
                token.value.type = VAL_INT;
                token.value.as.i_val = strtol(temp_num, NULL, 16);

                interpret->current_token = token;
                return;
            }
        }

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
                    set_error_state_interpret(interpret);
                    return;
                }

                has_decimal = true;
            }
            // Prevent temp buffer overflow
            if (temp_pos >= 64)
            {
                printf("Value Error: Given Number is too long!\n");
                set_error_state_interpret(interpret);
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
                set_error_state_interpret(interpret);
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
        case '|':
            make_single_char_token(interpret, BIT_OR);
            return;
        case '&':
            make_single_char_token(interpret, BIT_AND);
            return;
        case '^':
            make_single_char_token(interpret, BIT_XOR);
            return;
        case '~':
            make_single_char_token(interpret, BIT_NOT);
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
    set_error_state_interpret(interpret);
    return;
}

/*
 * ####################
 * #   Symbol Table   #
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
            // Check if variable exists, if it's constant, or add it to
            //  SymbolTable
            analyze_tree(node->right, symtab);
            define_symbol(symtab, node->token.name, false);
            break;

        case NODE_VAR:
            // Look up the variable in the SymbolTable to ensure it was
            // declared!
            lookup_symbol(symtab, node->token.name);
            break;

        // Numbers and Unary ops just pass through or get ignored by the
        // analyzer
        default:
            break;
    }
}

// Put a new symbol in the table if it does not exist or is not  const
static void define_symbol(SymbolTable *symtab, const char *name, bool is_const)
{
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

            return;
        }
    }
    if (symtab->count >= symtab->capacity)
    {
        symtab->capacity *= 2;
        Symbol *new_symbol = (Symbol *)realloc(
            symtab->symbols, symtab->capacity * sizeof(Symbol));

        if (new_symbol == NULL)
        {
            printf(
                "Fatal Error: Failed to allocate memory for symbol table!\n");
            set_error_state_symtab(symtab);
            return;
        }
        symtab->symbols = new_symbol;
    }
    unsigned int index = symtab->count;
    strcpy(symtab->symbols[index].name, name);
    symtab->symbols->is_const = is_const;
    symtab->count++;
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

    printf("Semantic Error: Variable '%s' is not defined!\n", name);
    symtab->error_found = true;
    return false;
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
                        set_error_state_interpret(interpret);
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
                        set_error_state_interpret(interpret);
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
                        set_error_state_interpret(interpret);
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
                        set_error_state_interpret(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }

                    if (left_val.as.i_val == INT_MIN &&
                        right_val.as.i_val == -1)
                    {
                        printf("Runtime Error: Integer Overflow\n");
                        set_error_state_interpret(interpret);
                        return (Value){VAL_INT, {.i_val = 0}};
                    }
                    double l_num = (double)left_val.as.i_val;
                    double r_num = (double)right_val.as.i_val;
                    return (Value){VAL_FLOAT, {.f_val = l_num / r_num}};
                }
                else if (node->token.type == BIT_OR)
                {
                    return (Value){
                        VAL_INT,
                        {.i_val = left_val.as.i_val | right_val.as.i_val}};
                }
                else if (node->token.type == BIT_XOR)
                {
                    return (Value){
                        VAL_INT,
                        {.i_val = left_val.as.i_val ^ right_val.as.i_val}};
                }
                else if (node->token.type == BIT_AND)
                {
                    return (Value){
                        VAL_INT,
                        {.i_val = left_val.as.i_val & right_val.as.i_val}};
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
                else
                {
                    printf("Runtime Error: Unallowed operator\n");
                    set_error_state_interpret(interpret);
                    return (Value){VAL_INT, {.i_val = 0}};
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
                    printf("Runtime Error: Cannot invert decimal number '%f\n'",
                           expr_val.as.f_val);
                    set_error_state_interpret(interpret);
                    return (Value){VAL_INT, {.i_val = 0}};
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

    interpret->mem_count = 0;
    interpret->mem_capacity = 8;

    interpret->memory = malloc(interpret->mem_capacity * sizeof(MemorySlot));

    if (interpret->memory == NULL)
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
    free(interpret->memory);
    interpret->memory = NULL;
    interpret->mem_count = 0;
    interpret->mem_capacity = 0;
}

// Check if a variable name is already known and if not save it as a new name
static void set_variable(Interpreter *interpret, const char *name, Value value)
{
    // Check if the variable already exist and if yes update it
    for (unsigned int i = 0; i < interpret->mem_count; i++)
    {
        if (strcmp(interpret->memory[i].name, name) == 0)
        {
            interpret->memory[i].value = value;
            return;
        }
    }

    if (interpret->mem_count >= interpret->mem_capacity)
    {
        interpret->mem_capacity *= 2;
        MemorySlot *new_memory = (MemorySlot *)realloc(
            interpret->memory, interpret->mem_capacity * sizeof(MemorySlot));

        if (new_memory == NULL)
        {
            printf("Fatal Error: Failed to allocate memory for variables!\n");
            set_error_state_interpret(interpret);
            return;
        }
        interpret->memory = new_memory;
    }
    unsigned int index = interpret->mem_count;
    strcpy(interpret->memory[index].name, name);
    interpret->memory[index].value = value;
    interpret->mem_count++;
}

// Check if a variable exists in the symbol table and if yes returns the value
static Value get_variable(Interpreter *interpret, const char *name)
{
    for (unsigned int i = 0; i < interpret->mem_count; i++)
    {
        if (strcmp(interpret->memory[i].name, name) == 0)
        {
            return interpret->memory[i].value;
        }
    }

    printf("Runtime Error: Variable '%s' is not defined!\n", name);
    set_error_state_interpret(interpret);
    return (Value){VAL_INT, {.i_val = 0}};
}

/*
 * ####################
 * # Helper Functions #
 * ####################
 */
// Helper functiion for adding single characters to a token
static void make_single_char_token(Interpreter *interpret, TokenTypes type)
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
static bool is_additive_op(TokenTypes type)
{
    return (bool)(type == PLUS || type == MINUS);
}

// Check if its a multiplication or dividing operator
static bool is_multiplicative_op(TokenTypes type)
{
    return (bool)(type == MUL || type == DIV);
}

// Check if its an assign operation
static bool is_assign_op(TokenTypes type) { return (bool)(type == ASSIGN); }
// Converts a single digit into a uint8_t number

// Safely injects a locked constant into the memory bank
static void set_constant(Interpreter *interpret, const char *name, Value value)
{
    set_variable(interpret, name, value);
}

// Get the next character without increasing the position of the interpreter
static char peek(Interpreter *interpret)
{
    unsigned int position = interpret->position + 1;
    if (position <= interpret->length)
    {
        return interpret->buffer[position];
    }
    return '\0';
}
