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
- [x] Floating point numbers (double precision) — *Done 2026-04-12*
- [x] Alternative bases (Hexadecimal `0x` and Binary `0b`) — *Done 2026-04-13*
- [x] Support for variables line `a = 10 * 5` — *Done 2026-04-12*
- [x] Support for bitwise operations like `|`, `&`, `~` and `^` — *Done 2026-04-13*
- [ ] Support for build in functions like `sin(90)`
- [ ] Semantic Analysis & Symbol Table: Introduce a compile-time analyzer phase to verify variable declarations and enforce rules before the run-time evaluator executes.
- [ ] Professional Error Reporting: Add line and column tracking to the Lexer to provide Clang-style, exact-location syntax diagnostics.
- [ ] Functions & The Call Stack: Upgrade the global memory architecture into a Stack of Activation Records to support local scoping and custom function calls.

## 🏗 Engine Architecture

The project follows a standard, state-driven compiler pipeline:

1. **Lexical Analysis (Lexer):** Scans the raw character buffer to emit strongly-typed `Token` structs, utilizing an isolated sandbox memory state to safely parse strings into integers.
2. **Recursive Descent Parser:** Consumes the token stream to dynamically construct an Abstract Syntax Tree (AST), ensuring mathematical precedence is mapped directly into the data structure.
3. **AST Evaluator:** Traverses the generated tree (Post-order) to compute the final expression, strictly protected by type-generic compiler extensions to trap Undefined Behavior.
4. **Context Encapsulation:** All execution state—including buffers, position pointers, and panic flags—is strictly isolated within a central `Interpreter` context struct, eliminating global variables.
