#ifndef _OVERFLOW_H_
#define _OVERFLOW_H_

// Check if the future C23 standard header exists
#if __has_include(<stdckint.h>)
  #include <stdckint.h>
  #define SAFE_ADD(a, b, res) ckd_add(res, a, b)
  #define SAFE_SUB(a, b, res) ckd_sub(res, a, b)
  #define SAFE_MUL(a, b, res) ckd_mul(res, a, b)
#elif defined(__GNUC__) || defined(__clang__)
  // Fallback to GCC/Clang built-ins
  #define SAFE_ADD(a, b, res) __builtin_add_overflow(a, b, res)
  #define SAFE_SUB(a, b, res) __builtin_sub_overflow(a, b, res)
  #define SAFE_MUL(a, b, res) __builtin_mul_overflow(a, b, res)
#else
  #warning "This compiler is not supported yes due the use of overflow checks!"
#endif

#endif // !_OVERFLOW_H_
