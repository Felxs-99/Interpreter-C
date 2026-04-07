#include "utest.h"
#include "interpreter.h"
#include <string.h>

typedef struct 
{
  int answer;
  bool error;
} TestResult;

// Helper function for the test to use the tests with the new ast structur
TestResult calc(char* math_string, size_t length)
{ 
  Interpreter interpret = {0}; 
  interpret.buffer = math_string;
  interpret.length = length;

  get_next_token(&interpret);
  ASTNode* tree = expr(&interpret);

  if (!interpret.error_found)
  {
    int final_answer = evaluate(tree, &interpret);

    if (!interpret.error_found)
    {
      free_ast(tree);
      return (TestResult) {.answer = final_answer, .error = false};
    }
  }
  // If an error was found return 0 and error true 
  free_ast(tree);
  return (TestResult) {.answer = 0, .error = true};
}

// Test Addition : Simplest case
UTEST(InterpreterTests, addition_single_digits_no_whitespace) {
  char test_expr[] = "2+2";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 4);
  ASSERT_FALSE(result.error);
}

// Test Addition: Whitespace handling
UTEST(InterpreterTests, addition_with_various_whitespace) {
  char test_expr[] = " 2  +  2 ";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 4);
  ASSERT_FALSE(result.error);
}

// Test Addition: Larger numbers
UTEST(InterpreterTests, addition_multiple_digits) {
  char test_expr[] = "100+250";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 350);
  ASSERT_FALSE(result.error);
}

// Test Subtraction : Simplest case
UTEST(InterpreterTests, subtraction_single_digits_no_whitespace) {
  char test_expr[] = "2-2";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 0);
  ASSERT_FALSE(result.error);
}

// Test Subtraction: Whitespace handling
UTEST(InterpreterTests, subtraction_with_various_whitespace) {
  char test_expr[] = " 2  -  2 ";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 0);
  ASSERT_FALSE(result.error);
}

// Test Subtraction: Larger numbers
UTEST(InterpreterTests, subtraction_multiple_digits) {
  char test_expr[] = "100-250";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, -150);
  ASSERT_FALSE(result.error);
}

// Test Multiplication : Simplest case
UTEST(InterpreterTests, multiplication_single_digits_no_whitespace) {
  char test_expr[] = "2*3";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 6);
  ASSERT_FALSE(result.error);
}

// Test Multiplication: Whitespace handling
UTEST(InterpreterTests, multiplication_with_various_whitespace) {
  char test_expr[] = " 2  *  3 ";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 6);
  ASSERT_FALSE(result.error);
}

// Test Multiplication: Larger numbers
UTEST(InterpreterTests, multiplication_multiple_digits) {
  char test_expr[] = "100*250";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 25000);
  ASSERT_FALSE(result.error);
}

// Test Division : Simplest case
UTEST(InterpreterTests, division_single_digits_no_whitespace) {
  char test_expr[] = "4/2";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 2);
  ASSERT_FALSE(result.error);
}

// Test Division: Whitespace handling
UTEST(InterpreterTests, division_with_various_whitespace) {
  char test_expr[] = " 4  /  2 ";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 2);
  ASSERT_FALSE(result.error);
}

// Test Division: Larger numbers
UTEST(InterpreterTests, divison_multiple_digits) {
  char test_expr[] = "2500/100";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 25);
  ASSERT_FALSE(result.error);
}

// Test Combination: Combination of +, -, *, /
UTEST(InterpreterTests, combination_multiple_signs)
{
  char test_expr[] = "14 + 2 * 3 - 6 / 2";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 17);
  ASSERT_FALSE(result.error);
}

// Test Parentheses: One ()
UTEST(InterpreterTests, parentheses_one)
{
  char test_expr[] = "2 * (5 + 1)";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 12);
  ASSERT_FALSE(result.error);
}

// Test Parentheses: Multiple ()
UTEST(InterpreterTests, parentheses_multiple)
{
  char test_expr[] = "7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 10);
  ASSERT_FALSE(result.error);
}

// Test Unary Operations: Simple Operaton
UTEST(InterpreterTests, unary_operator_simple)
{
  char test_expr[] = "-1 + 2";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, 1);
  ASSERT_FALSE(result.error);
}
// Test Unary Operations: Complex Operaton
UTEST(InterpreterTests, unary_operator_complex)
{
  char test_expr[] = "10 * - 1 + -2 * (13 ++2)";
  TestResult result = calc(test_expr, sizeof(test_expr));
  ASSERT_EQ(result.answer, -40);
  ASSERT_FALSE(result.error);
}

UTEST_MAIN();
