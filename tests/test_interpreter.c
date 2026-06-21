#include "interpreter.h"
#include "utest.h"
#include <math.h>
#include <string.h>

typedef struct
{
    Value answer;
    bool error;
} TestResult;

UTEST_MAIN();

// Helper function for the test to use the tests with the new ast structur
TestResult calc(char *math_string, size_t length)
{
    Interpreter interpret = {0};
    interpret.buffer = math_string;
    interpret.length = length;

    SymbolTable symtab = {0};
    init_symtab(&symtab);

    get_next_token(&interpret);
    ASTNode *tree = statement(&interpret);

    if (interpret.error_found || tree == NULL)
    {
        free_ast(tree);

        return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                            .error = true};
    }

    analyze_tree(tree, &symtab);

    if (symtab.error_found)
    {
        free_ast(tree);
        symtab.error_found = false; // Reset the flag so the next line works!
        return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                            .error = true};
    }

    if (!interpret.error_found)
    {
        Value final_answer = evaluate(tree, &interpret);

        if (!interpret.error_found)
        {
            free_ast(tree);
            free_interpreter(&interpret);
            return (TestResult){.answer = final_answer, .error = false};
        }
    }
    // If an error was found return 0 and error true
    free_ast(tree);
    free_interpreter(&interpret);
    return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                        .error = true};
}

// Runs an array of strilngs through a single interpreter state
TestResult calc_script(const char **lines, int line_count)
{
    Interpreter interpret = {0};
    init_interpreter(&interpret);

    SymbolTable symtab = {0};
    init_symtab(&symtab);

    // 1. Initialize final_answer as a Value struct
    Value final_answer = {VAL_INT, {.i_val = 0}};

    for (int i = 0; i < line_count; i++)
    {
        reset_interpreter_line(&interpret, (char *)lines[i]);
        get_next_token(&interpret);
        ASTNode *tree = statement(&interpret);

        if (interpret.error_found)
        {
            free_ast(tree);
            free_interpreter(&interpret);
            // 2. Return empty struct on error
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        analyze_tree(tree, &symtab);

        if (symtab.error_found)
        {
            free_ast(tree);
            symtab.error_found =
                false; // Reset the flag so the next line works!
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        final_answer = evaluate(tree, &interpret);

        if (interpret.error_found)
        {
            free_ast(tree);
            free_interpreter(&interpret);
            // 3. Return empty struct on error
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        free_ast(tree);
    }

    free_interpreter(&interpret);
    return (TestResult){.answer = final_answer, .error = false};
}

/*
 * ####################
 * #   Positv Tests   #
 * ####################
 */

// Test Addition : Simplest case
UTEST(InterpreterTests, addition_single_digits_no_whitespace)
{
    char test_expr[] = "2+2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 4);
}

// Test Addition: Whitespace handling
UTEST(InterpreterTests, addition_with_various_whitespace)
{
    char test_expr[] = " 2  +  2 ";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 4);
}

// Test Addition: Larger numbers
UTEST(InterpreterTests, addition_multiple_digits)
{
    char test_expr[] = "100+250";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 350);
}

// Test Subtraction : Simplest case
UTEST(InterpreterTests, subtraction_single_digits_no_whitespace)
{
    char test_expr[] = "2-2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 0);
}

// Test Subtraction: Whitespace handling
UTEST(InterpreterTests, subtraction_with_various_whitespace)
{
    char test_expr[] = " 2  -  2 ";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 0);
}

// Test Subtraction: Larger numbers
UTEST(InterpreterTests, subtraction_multiple_digits)
{
    char test_expr[] = "100-250";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, -150);
}

// Test Multiplication : Simplest case
UTEST(InterpreterTests, multiplication_single_digits_no_whitespace)
{
    char test_expr[] = "2*3";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 6);
}

// Test Multiplication: Whitespace handling
UTEST(InterpreterTests, multiplication_with_various_whitespace)
{
    char test_expr[] = " 2  *  3 ";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 6);
}

// Test Multiplication: Larger numbers
UTEST(InterpreterTests, multiplication_multiple_digits)
{
    char test_expr[] = "100*250";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 25000);
}

// Test Division : Simplest case
UTEST(InterpreterTests, division_single_digits_no_whitespace)
{
    char test_expr[] = "4/2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_FLOAT);
    ASSERT_TRUE(fabs(2.0 - result.answer.as.f_val) < 0.0001);
}

// Test Division: Whitespace handling
UTEST(InterpreterTests, division_with_various_whitespace)
{
    char test_expr[] = " 4  /  2 ";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_FLOAT);
    ASSERT_TRUE(fabs(2.0 - result.answer.as.f_val) < 0.0001);
}

// Test Division: Larger numbers
UTEST(InterpreterTests, divison_multiple_digits)
{
    char test_expr[] = "2500/100";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_FLOAT);
    ASSERT_TRUE(fabs(25.0 - result.answer.as.f_val) < 0.0001);
}

// Test Combination: Combination of +, -, *, /
UTEST(InterpreterTests, combination_multiple_signs)
{
    char test_expr[] = "14 + 2 * 3 - 6 / 2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_FLOAT);
    ASSERT_TRUE(fabs(17.0 - result.answer.as.f_val) < 0.0001);
}

// Test Parentheses: One ()
UTEST(InterpreterTests, parentheses_one)
{
    char test_expr[] = "2 * (5 + 1)";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 12);
}

// Test Parentheses: Multiple ()
UTEST(InterpreterTests, parentheses_multiple)
{
    char test_expr[] =
        "7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_FLOAT);
    ASSERT_TRUE(fabs(10.0 - result.answer.as.f_val) < 0.0001);
}

// Test Unary Operations: Simple Operaton
UTEST(InterpreterTests, unary_operator_simple)
{
    char test_expr[] = "-1 + 2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 1);
}
// Test Unary Operations: Complex Operaton
UTEST(InterpreterTests, unary_operator_complex)
{
    char test_expr[] = "10 * - 1 + -2 * (13 ++2)";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, -40);
}

// Test Variables: Assign a value to a variable
UTEST(InterpreterTests, variable_assignment_simple)
{
    const char *test_script[] = {"x = 10", "x"};

    TestResult result = calc_script(test_script, 2);

    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 10);
}

// Test Variables: Assign a complex expression a variable
UTEST(InterpreterTests, variable_assignment_complex)
{
    const char *test_script[] = {"x = (2+3)*100 - 5", "x"};

    TestResult result = calc_script(test_script, 2);

    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 495);
}

// Test Variables: Calculate with a single variable
UTEST(InterpreterTests, variable_calculate_single)
{
    const char *test_script[] = {"x = 10", "x * 100"};

    TestResult result = calc_script(test_script, 2);

    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 1000);
}

// Test Variables: Calculate with a multiple variable
UTEST(InterpreterTests, variable_calculate_multiple)
{
    const char *test_script[] = {"x = 10", "y = 20", "x + y"};

    TestResult result = calc_script(test_script, 3);

    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 30);
}

// Test Other numerical Bases: Binary numbers
UTEST(InterpreterTests, other_bases_binary)
{
    char test_expr[] = "0b0011";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 3);
}

// Test Other numerical Bases: Hex numbers
UTEST(InterpreterTests, other_bases_hex)
{
    char test_expr[] = "0xFF";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 255);
}

// Test Bitwise Operations: Or
UTEST(InterpreterTests, bitwise_op_or)
{
    char test_expr[] = "2|1";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 3);
}

// Test Bitwise Operations: Xor
UTEST(InterpreterTests, bitwise_op_xor)
{
    char test_expr[] = "3^2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 1);
}

// Test Bitwise Operations: And
UTEST(InterpreterTests, bitwise_op_and)
{
    char test_expr[] = "7&3";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 3);
}

// Test Bitwise Boolean Operations: Or
UTEST(InterpreterTests, bitwise_op_or_bool)
{
    char test_expr[] = "false|true";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, 1);
}

// Test Bitwise Boolean Operations: Xor
UTEST(InterpreterTests, bitwise_op_xor_bool)
{
    char test_expr[] = "false^true";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, 1);
}

// Test Bitwise Boolean Operations: And
UTEST(InterpreterTests, bitwise_op_and_bool)
{
    char test_expr[] = "false&false";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, 0);
}

// Test Keywords: Constants
UTEST(InterpreterTests, keywords_const)
{
    const char *test_script[] = {"const x = 10", "x + 20"};
    TestResult result = calc_script(test_script, 2);
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.b_val, 30);
}

// Test Precedence: Basic level precedence (should be always the last in the
// positiv test to find it better)
UTEST(InterpreterTests, basic_precedece)
{
    char test_expr[] = "2 | 3 ^ 4 & 5 + 1 * 2";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 7);
}

// Test Precedence: Advanced level precedence (should be always the last in the
// positiv test to find it better)
UTEST(InterpreterTests, advanced_precedence)
{
    char test_expr[] = "~(1 ^ 3) & 15 | 8";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 13);
}

// Test Precedence: Math vs Relational
// Math should happen BEFORE the comparison: (10 + (2 * 3)) > ((4 * 3) + 3) -->
// 16 > 15 --> true
UTEST(InterpreterTests, precedence_math_vs_relational)
{
    char test_expr[] = "10 + 2 * 3 > 4 * 3 + 3";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, true);
}

// Test Precedence: Relational vs Equality
// Relational (<, >) should happen BEFORE Equality (==, !=): (5 > 3) == (4 < 10)
// --> true == true --> true
UTEST(InterpreterTests, precedence_relational_vs_equality)
{
    char test_expr[] = "5 > 3 == 4 < 10";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, true);
}

// Test Precedence: The Famous C Trap (Equality vs Bitwise AND)
// Equality (==) happens BEFORE Bitwise AND (&).
// Evaluates as: (7 == (3 + 4)) & (5 == 5) --> (7 == 7) & true --> true & true
// --> true
UTEST(InterpreterTests, precedence_the_c_trap)
{
    char test_expr[] = "7 == 3 + 4 & 5 == 5";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, true);
}

// Test Precedence: Boolean Unary vs Equality vs Bitwise OR
// Evaluates as: ((~false) == true) | false --> (true == true) | false --> true
// ~false --> true
UTEST(InterpreterTests, precedence_boolean_logic_chain)
{
    char test_expr[] = "~false == true | false";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_BOOL);
    ASSERT_EQ(result.answer.as.b_val, true);
}

// Test Precedence: Complex Bitwise Math
// & happens before ^, which happens before |
// Evaluates as: 1 | (2 ^ (3 & 4)) --> 1 | (2 ^ 0) --> 1 | 2 --> 3
UTEST(InterpreterTests, precedence_complex_bitwise)
{
    char test_expr[] = "1 | 2 ^ 3 & 4";
    TestResult result = calc(test_expr, sizeof(test_expr));
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 3);
}

/*
 * ####################
 * #   If/Else Tests  #
 * ####################
 */

// Helper: runs a full multi-line block through the interpreter
static TestResult calc_block(const char *src)
{
    size_t len = strlen(src);
    char *buffer = malloc(len + 1);
    memcpy(buffer, src, len + 1);

    Interpreter interpret = {0};
    init_interpreter(&interpret);

    SymbolTable symtab = {0};
    init_symtab(&symtab);

    Value final_answer = {VAL_INT, {.i_val = 0}};

    reset_interpreter_line(&interpret, buffer);
    get_next_token(&interpret);

    while (interpret.current_token.type != EOF_TOKEN && !interpret.error_found)
    {
        if (interpret.current_token.type == EOL)
        {
            get_next_token(&interpret);
            continue;
        }

        ASTNode *tree = statement(&interpret);

        if (interpret.error_found || tree == NULL)
        {
            free_ast(tree);
            free_interpreter(&interpret);
            free(buffer);
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        analyze_tree(tree, &symtab);

        if (symtab.error_found)
        {
            free_ast(tree);
            symtab.error_found = false;
            free_interpreter(&interpret);
            free(buffer);
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        final_answer = evaluate(tree, &interpret);

        if (interpret.error_found)
        {
            free_ast(tree);
            free_interpreter(&interpret);
            free(buffer);
            return (TestResult){.answer = (Value){VAL_INT, {.i_val = 0}},
                                .error = true};
        }

        free_ast(tree);
    }

    free_interpreter(&interpret);
    free(buffer);
    return (TestResult){.answer = final_answer, .error = false};
}

// if true inline syntax — body executes, returns body result
UTEST(IfTests, if_true_inline)
{
    TestResult result = calc_block("if true { 42 }");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 42);
}

// if false inline syntax — body skipped, returns 0
UTEST(IfTests, if_false_inline)
{
    TestResult result = calc_block("if false { 42 }");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 0);
}

// if true multiline syntax
UTEST(IfTests, if_true_multiline)
{
    TestResult result = calc_block("if true\n{\n42\n}");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 42);
}

// if/else — true branch taken
UTEST(IfTests, if_else_true_branch)
{
    TestResult result = calc_block("if true { 1 } else { 2 }");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 1);
}

// if/else — else branch taken
UTEST(IfTests, if_else_false_branch)
{
    TestResult result = calc_block("if false { 1 } else { 2 }");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 2);
}

// if/else multiline — else branch taken
UTEST(IfTests, if_else_multiline_false_branch)
{
    TestResult result = calc_block("if false\n{\n1\n}\nelse\n{\n2\n}");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 2);
}

// condition is a boolean expression, not a literal
UTEST(IfTests, if_boolean_expression_condition)
{
    TestResult result = calc_block("if 3 > 2 { 99 }");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 99);
}

// variable assigned inside body is accessible after the block
UTEST(IfTests, if_body_variable_persists)
{
    TestResult result = calc_block("if true { x = 7 }\nx");
    ASSERT_FALSE(result.error);
    ASSERT_EQ((int)result.answer.type, VAL_INT);
    ASSERT_EQ(result.answer.as.i_val, 7);
}

// non-boolean condition triggers runtime error
UTEST(IfTests, if_non_boolean_condition_errors)
{
    TestResult result = calc_block("if 5 { 1 }");
    ASSERT_TRUE(result.error);
}

/*
 * ####################
 * #   Negativ Tests  #
 * ####################
 */

// Test Syntax Error: Missing int, unary or '('
// UTEST(InterpreterTests, syntax_missing_uap)
//{
//    char test_expr[] = "2+";
//    TestResult result = calc(test_expr, sizeof(test_expr));
//    ASSERT_TRUE(result.error);
//}
