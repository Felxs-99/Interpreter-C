#include "utest.h"
#include "interpreter.h"
#include <string.h>

// Test Addition : Simplest case
UTEST(InterpreterTests, addition_single_digits_no_whitespace) {
    char test_expr[] = "2+2";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 4);
}

// Test Addition: Whitespace handling
UTEST(InterpreterTests, addition_with_various_whitespace) {
    char test_expr[] = " 2  +  2 ";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 4);
}

// Test Addition: Larger numbers
UTEST(InterpreterTests, addition_multiple_digits) {
    char test_expr[] = "100+250";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 350);
}

// Test Subtraction : Simplest case
UTEST(InterpreterTests, subtraction_single_digits_no_whitespace) {
    char test_expr[] = "2-2";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 0);
}

// Test Subtraction: Whitespace handling
UTEST(InterpreterTests, subtraction_with_various_whitespace) {
    char test_expr[] = " 2  -  2 ";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 0);
}

// Test Subtraction: Larger numbers
UTEST(InterpreterTests, subtraction_multiple_digits) {
    char test_expr[] = "100-250";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, -150);
}

// Test Multiplication : Simplest case
UTEST(InterpreterTests, multiplication_single_digits_no_whitespace) {
    char test_expr[] = "2*3";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 6);
}

// Test Multiplication: Whitespace handling
UTEST(InterpreterTests, multiplication_with_various_whitespace) {
    char test_expr[] = " 2  *  3 ";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 6);
}

// Test Multiplication: Larger numbers
UTEST(InterpreterTests, multiplication_multiple_digits) {
    char test_expr[] = "100*250";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 25000);
}

// Test Division : Simplest case
UTEST(InterpreterTests, division_single_digits_no_whitespace) {
    char test_expr[] = "4/2";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 2);
}

// Test Division: Whitespace handling
UTEST(InterpreterTests, division_with_various_whitespace) {
    char test_expr[] = " 4  /  2 ";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 2);
}

// Test Division: Larger numbers
UTEST(InterpreterTests, divison_multiple_digits) {
    char test_expr[] = "2500/100";
    int result = calc(test_expr, sizeof(test_expr));
    ASSERT_EQ(result, 25);
}


UTEST_MAIN();
