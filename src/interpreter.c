#include "interpreter.h"
#include <stdbool.h>
#include <stdio.h>

static int multiple_digit_number(int number, int digit_to_add);
static int calc_last_two_numbers(int first_number, int second_number,
                                 token_types operator);

// Evaluate the expression
int expr(char* buffer, size_t length)
{
    Interpreter interpret = {0};
    interpret.buffer = buffer;
    interpret.length = length;
    interpret.position = 0;
    get_next_token(&interpret);

    int result = 0;
    token_types last_token_type = NONE;

    Token new_token;

    while (true)
    {
        if (interpret.error_found)
        {
            printf("Token Error occured!\n");
            break;
        }

        new_token = interpret.current_token;

        if (new_token.type == EOL)
        {
            break;
        }
        else if (new_token.type == SPACE)
        {
            if (last_token_type == NONE)
            {
                printf("Space at the beginning is not allowed!\n");
                break;
            }

            if (!eat(SPACE, &interpret))
            {
                printf("Operator Error occured!\n");
                break;
            }
            continue;
        }
        else if (last_token_type == INT)
        {
            // After an INT, we expect an operator
            if (new_token.type == PLUS)
            {
                eat(PLUS, &interpret);
            }
            else if (new_token.type == MINUS)
            {
                eat(MINUS, &interpret);
            }
            else
            {
                printf("Operator Error: Expected + or -\n");
                break;
            }
        }
        else if (last_token_type == PLUS || last_token_type == MINUS)
        {
            // After an operator, we expect an INT and do the math
            token_types op = last_token_type;

            if (!eat(INT, &interpret))
            {
                printf("Integer Error occured!\n");
                break;
            }
            else
            {
                if (op == PLUS)
                {
                    result += new_token.value;
                }
                else
                {
                    result -= new_token.value;
                }
            }
        }
        else if (last_token_type == NONE)
        {
            // Initial state: we expect the first number
            if (!eat(INT, &interpret))
            {
                printf("Integer needed Error occured!\n");
                break;
            }
            else
            {
                result = new_token.value;
            }
        }
        else
        {
            printf("Error\n");
            break;
        }

        last_token_type = new_token.type;
    }

    return result;
}

// Create the token for the next character
void get_next_token(Interpreter* interpret)
{
    Token token = {0};

    // Return a EOL token when the last character of the buffer is reached \0 or \n
    if (interpret->position > (interpret->length - 1))
    {
        token.type = EOL;
        token.value = 0;
    }
    else
    {
        while (true)
        {

            if (interpret->position > interpret->length)
            {
                interpret->error_found = true;
                break;

            }
            char current_char = interpret->buffer[interpret->position];

            if (current_char == ' ')
            {
                interpret->position += 1;
                continue;
            }
            else  if (is_digit(current_char))
            {
                token.type = INT;
                token.value = convert_char(current_char);
                interpret->position += 1;
            }

            else if (current_char == '+')
            {
                token.type = PLUS;
                token.value = (int)current_char;
                interpret->position += 1;
            }
            else if (current_char == '-')
            {
                token.type = MINUS;
                token.value = (int)current_char;
                interpret->position += 1;
            }
            else if (current_char == '\n')
            {
                token.type = EOL;
            }
            else
            {
                // Nothing fits -> returns the error
                interpret->error_found = true;
            }
            break;
        }

        }

    interpret->current_token = token;
}

bool eat(token_types token, Interpreter* interprete)
{
    if (interprete->current_token.type == token)
    {
        get_next_token(interprete);
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
    if ((character >= '0') && (character <= '9'))
    {
        return true;
    }
    else
    {
        return false;
    }

}

// Converts a single digit into a uint8_t number
uint8_t convert_char(char character)
{
    return (uint8_t) (character - '0');
}

// Add the next lower digit to a number
static int multiple_digit_number(int number, int digit_to_add)
{
    return (number * 10) + digit_to_add;
}
