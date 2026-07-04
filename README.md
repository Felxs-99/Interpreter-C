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
- **Control Flow:** `if`/`else` and `while` with `{}` block syntax, supporting both inline and multiline forms.
- **Functions:** First-class `def` functions with parameter passing, local scopes, and `return()` values.
- **Recursion:** Runtime scope chain (`RuntimeScope` linked list) enables full recursive calls with isolated per-call state.
- **Explicit Output:** `print()` statement for controlled output — file mode is silent by default.
- **Interactive REPL:** CLI with readline history and smart `...` prompt for multi-line block input.
- **Built-in Math Functions:** `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `floor`, `ceil`, `abs`, `ln`, `log`, `pow` — with type-aware `pow` using exact integer arithmetic for integer exponents and `double` fallback otherwise.
- **Dynamic Identifiers:** Identifiers are heap-allocated with realloc growth — no length limit on variable or function names.
- **Built-in Name Protection:** Semantic analysis rejects variable assignments or function definitions that shadow built-in names.

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
- [x] Support for reserved keywords like `const` — *Done 2026-04-15*
- [x] Semantic Analysis & Symbol Table: Introduce a compile-time analyzer phase to enforce rules before the run-time evaluator executes. — *Done 2026-04-15*
- [x] Support larger numbers — *Done 2026-04-16*
- [x] `if`/`else` control flow with `{}` block syntax — *Done 2026-06-21*
- [x] `while` loop — *Done 2026-06-21*
- [x] `print()` built-in statement — *Done 2026-06-21*
- [x] CLI multi-line block input with brace depth tracking — *Done 2026-06-21*
- [x] Scoped symbol table (nested scopes via `enclosing_scope` pointer) — *Done 2026-06-29*
- [x] Functions (`def` keyword) with local scope and `return()` — *Done 2026-06-29*
- [x] Recursion via `RuntimeScope` call-frame chain — *Done 2026-06-29*
- [x] Built-in math functions (`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `floor`, `ceil`, `abs`, `ln`, `log`, `pow`) — *Done 2026-07-04*
- [x] Dynamic identifier allocation — no length limit on names — *Done 2026-07-04*
- [ ] Semantic Analysis: type checking in symbol table
- [ ] Professional Error Reporting: line and column tracking for Clang-style diagnostics
- [ ] Functions & The Call Stack: Stack of Activation Records for local scoping

## 🏗 Engine Architecture

The project follows a standard, state-driven compiler pipeline:

1. **Lexical Analysis (Lexer):** Scans the raw character buffer to emit strongly-typed `Token` structs, utilizing an isolated sandbox memory state to safely parse strings into integers.
2. **Recursive Descent Parser:** Consumes the token stream to dynamically construct an Abstract Syntax Tree (AST), ensuring mathematical precedence is mapped directly into the data structure.
3. **Semantic Analysis (Symbol Table):** A compile-time pass walks the AST before evaluation to enforce scoping rules, catch undefined variables, validate function signatures, and analyze function bodies in isolated child scopes.
4. **AST Evaluator:** Traverses the generated tree (Post-order) to compute the final expression, strictly protected by type-generic compiler extensions to trap Undefined Behavior.
5. **Runtime Scope Chain:** Function calls allocate a `RuntimeScope` frame on a linked list, binding arguments as local variables. The chain is walked for variable lookup, enabling full lexical scoping and recursion.
6. **Context Encapsulation:** All execution state—including buffers, position pointers, and panic flags—is strictly isolated within a central `Interpreter` context struct, eliminating global variables.
