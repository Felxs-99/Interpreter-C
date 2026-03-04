#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// Available operators and EOL -> end of line
typedef enum
{
    INT = 0,
    PLUS,
    EOL,
    ERROR,
} token_types;

// Token struct
typedef struct {
  int value;
  token_types type;
} Token;

int expr(char* buffer, size_t lenght);
Token get_next_token(char* buffer, size_t* current_pos, size_t length);
uint8_t convert_char(char character);
bool is_digit(char character);
bool eat(token_types token, Token current_token);

int main()
{
    // Input buffer for user input, can hold 49 chars + \0
    size_t length = 0;
    char input_buffer[50] = {0};
    int c = 0;
    while (1)
    {
        printf(">>>");
        if (fgets(input_buffer, sizeof(input_buffer), stdin))
        {
            length = strlen(input_buffer);
            if (strchr(input_buffer, '\n') == NULL)
            {
                // Ignore the line with too much input characters
                while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }
        }
        printf("%d\n", expr(input_buffer, length));
    }
    return 0;
}

// Evaluate the expression
int expr(char* buffer, size_t length)
{
    size_t current_position = 0;
    Token current_token = {0};
    current_token = get_next_token(buffer, &current_position, length);

    Token left = current_token;
    if (!eat(INT, current_token))
    {
        printf("Syntax Error occured!\n");
    }
    current_token =  get_next_token(buffer,&current_position, length);

    Token op = current_token;
    if (!eat(PLUS, current_token))
    {
        printf("Syntax Error occured!\n");
    }
    current_token =  get_next_token(buffer,&current_position, length);
    Token right = current_token;
    if (!eat(INT, current_token))
    {
        printf("Syntax Error occured!\n");
    }

    return left.value + right.value;
}

// Create the token for the next character
Token get_next_token(char* buffer, size_t* current_pos, size_t length)
{
    Token token = {0};

    // Return a EOL token when the last character of the buffer is reached \0 or \n
    if (*current_pos > (length - 1))
    {
        token.type = EOL;
        token.value = 0;
    }

    char current_char = buffer[*current_pos];

    if (is_digit(current_char))
    {
        token.type = INT;
        token.value = convert_char(current_char);
        *current_pos += 1;
        return token;
    }

    if (current_char == '+')
    {
        token.type = PLUS;
        token.value = (int)current_char;
        *current_pos += 1;
        return token;
    }

    // Nothing fits -> returns the error as token
    token.type = ERROR;
    token.value = 0;
    return token;
}

bool eat(token_types token, Token current_token)
{
    if (current_token.type == token)
    {
        return true;
    }
    return false;
}

/*
 * Check if the character is a digit (0 - 9)
 * Returns true, if it is a digit, false if not
 */
bool is_digit(char character)
{
    if ((character > '0') && (character < '9'))
    {
        return true;
    }
    else
    {
        return false;
    }

}

uint8_t convert_char(char character)
{
    return (uint8_t) (character - '0');
}
