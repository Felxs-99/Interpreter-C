# Interpreter-C: A Simple Expression Interpreter

A lightweight, handwritten interpreter built in C to explore the fundamentals of Lexical Analysis and Parsing. 

Inspired by the [Let’s Build A Simple Interpreter](https://ruslanspivak.com/lsbasi-part1/) series.

## 🚀 Features
- **Handwritten Lexer:** Tokenizes input strings into integers and operators.
- **State-Machine Parser:** Evaluates linear expressions with support for unary signs (e.g., `-1 + 2`).
- **Whitespace Agnostic:** Correct handling of spaces between operands and operators.
- **Multi-digit Support:** Correctly gathers sequences of digits into single numeric values.

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
