# Interpreter-C: A Simple Expression Interpreter

A lightweight, handwritten interpreter built in C to explore the fundamentals of Lexical Analysis and Parsing. 

Inspired by the [Let’s Build A Simple Interpreter](https://ruslanspivak.com/lsbasi-part1/) series.

## 🚀 Features
- **Memory-Safe Tokenization:** Handwritten lexer featuring safe-state buffering to intercept integer bounds violations prior to evaluation.
- **AST-Driven Evaluation:** Replaced the linear parser with a Recursive Descent architecture, naturally enforcing strict operator precedence via an Abstract Syntax Tree.
- **Parenthetical Grouping:** Full support for complex, nested sub-expressions.
- **UB-Free Arithmetic:** Completely eliminates C's Undefined Behavior during math operations using `__builtin` compiler extensions to trap hardware-level overflow, underflow, and division-by-zero.
- **Leak-Free Panic Mode:** Robust error recovery that detects invalid syntax, safely unwinds and frees the dynamically allocated AST, and returns precise diagnostics.
- **Forgiving Syntax:** 100% whitespace agnostic.

## 🛠 Project Roadmap
- [x] Basic addition and subtraction (`1 + 1`, `-5 + 10`) — *Done 2026-03-05*
- [x] Multiplication and Division — *Done 2026-04-04*
- [x] Convert from *syntax-directed* to an *ast* interpreter — *Done 2026-04-07*
- [x] Operator Precedence (BODMAS/PEMDAS) — *Done 2026-04-07*
- [x] Parentheses for nested expressions — *Done 2026-04-07*
- [ ] Floating point numbers (double precision)
- [ ] Alternative bases (Hexadecimal `0x` and Binary `0b`)
- [ ] Support for variables line `a = 10 * 5`
- [ ] Support for build in functions like `sin(90)`

## 🏗 Architecture
The project follows a classic interpreter pipeline:
1. **Lexer:** Scans the raw buffer and produces `Token` structs.
2. **Parser:** Consumes tokens and calculates results using a recursive logic flow.
3. **Interpreter State:** Encapsulated in a central `Interpreter` struct to avoid global variables.
