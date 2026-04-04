#include "interpreter.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

static void get_next_token(Interpreter* interpret);
static uint8_t convert_char(char character);
static bool eat(token_types token, Interpreter *interprete);
static int factor(Interpreter *interpret);
static int term(Interpreter *interpret);
static int multiple_digit_number(int number, int digit_to_add);
static void emit_single_char_token(Interpreter *interpret, token_types type,
                                   int value);
static int expr(Interpreter* interpret);
static bool is_additive_op(token_types type);
static bool is_multiplicative_op(token_types type);

int calc(char* buffer, size_t length)
{
  Interpreter interpret = {0};
  interpret.buffer = buffer;
  interpret.length = length;
  interpret.position = 0;
  get_next_token(&interpret);
  return expr(&interpret);
}

// Interpreter
// Evaluate the expression
static int expr(Interpreter* interpret)
{

  int result = term(interpret);
  Token token = {0};

  while (is_additive_op(interpret->current_token.type))
  {
    // Stop evaluating if a syntax error was found in factor()
    if (interpret->error_found) break;
    token = interpret->current_token;

    if (token.type == PLUS)
    {
      eat(PLUS, interpret);
      result = result + term(interpret);
    }
    else if (token.type == MINUS)
    {
      eat(MINUS, interpret);
      result = result - term(interpret);
    }
  }
  return result;
}


// Eat the provided token
static bool eat(token_types token, Interpreter* interpret)
{
  if (interpret->current_token.type == token)
  {
    get_next_token(interpret);
    return true;
  }
  return false;
}

// factor : INTEGER
static int factor(Interpreter *interpret)
{
  Token token = interpret->current_token;
  if (token.type == INT)
  {
    eat(INT, interpret); // We know this is true, safe to ignore return
    return token.value;
  }
  else if (token.type == LPAREN)
  {
    eat(LPAREN, interpret);
    int result = expr(interpret);
    eat(RPAREN, interpret);
    return result;
  }
  else
  {
    // ERROR CASE: The token was not an integer!
    printf("Syntax Error: Expected an Integer\n");
    interpret->error_found = true;
    return 0;
  }
}

// term : factor ((MUL | DIV) factor)*
static int term(Interpreter *interpret)
{
  int result = factor(interpret);
  Token token = {0};

  while (is_multiplicative_op(interpret->current_token.type))
  {
    // Stop evaluating if a syntax error was found in factor()
    if (interpret->error_found) break;
    token = interpret->current_token;

    if (token.type == MUL)
    {
      eat(MUL, interpret);
      result = result * factor(interpret);
    }
    else if (token.type == DIV)
    {
      eat(DIV, interpret);
      int right_factor = factor(interpret);

      // Check for cascading errors before checking for div-by-zero
      if (interpret->error_found) break;

      if (right_factor == 0)
      {
        printf("Runtime Error: Division by zero\n");
        interpret->error_found = true;
        break;
      }

      result = result / right_factor;
    }
  }
  return result;
}

// Lexer
// Create the token for the next character
static void get_next_token(Interpreter* interpret)
{
  // Create an empty token
  Token token = {0};
  char current_char = interpret->buffer[interpret->position];

  // Detect whitespaces and skip them, relies on the fact that buffer is 0 terminated
  while (current_char == ' ')
  {
    interpret->position++;
    current_char = interpret->buffer[interpret->position];
  }

  // Return a EOL token when the last character of the buffer is reached \0 or \n
  if ((interpret->position > (interpret->length - 1)) || (current_char == '\n') || (current_char == '\0') )
  {
    emit_single_char_token(interpret, EOL, 0);
    return;
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
      token.value = multiple_digit_number(
        token.value,
        convert_char(interpret->buffer[interpret->position]));
      interpret->position ++;
    }

    interpret->current_token = token;
    return;
  }

  // Check if its a plus sign
  if (current_char == '+')
  {
    emit_single_char_token(interpret, PLUS, 0);
    return;
  }


  // Check if its a minus sign
  if (current_char == '-')
  {
    emit_single_char_token(interpret, MINUS, 0);
    return;
  }

  // Check if its a asterix sign
  if (current_char == '*')
  {
    emit_single_char_token(interpret, MUL, 0);
    return;
  }

  // Check if its a divison sign
  if (current_char == '/')
  {
    emit_single_char_token(interpret, DIV, 0);
    return;
  }

  // Check if its a left parentheses sign
  if (current_char == '(')
  {
    emit_single_char_token(interpret, LPAREN, 0);
    return;
  }

  // Check if its a right parentheses sign
  if (current_char == ')')
  {
    emit_single_char_token(interpret, RPAREN, 0);
    return;
  }

  // Unknow character
  interpret->error_found = true;
  return;
}


// Helper functions
// Helper functiion for adding single characters to a token
static void emit_single_char_token(Interpreter* interpret, token_types type, int value)
{
  Token token = {0};
  token.type = type;
  token.value = value;

  // Save to the interpreter state
  interpret->current_token = token;

  // Do the annoying position increment here, once!
  interpret->position++;
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
// Converts a single digit into a uint8_t number
static uint8_t convert_char(char character)
{
  return (uint8_t) (character - '0');
}

// Add the next lower digit to a number
static int multiple_digit_number(int number, int digit_to_add)
{
  return (number * 10) + digit_to_add;
}
