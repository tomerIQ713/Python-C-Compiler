Python-C-Compiler

A compiler that translates Python to C. Implemented in pure C.

This project demonstrates the core principles of compiler design, including lexical analysis, parsing, semantic checking, and code translation from a high-level language (Python-like syntax) into C code. It focuses on understanding how programming languages are processed and transformed into executable form.

The system is built with modular components such as a parser, translation context, and token processing pipeline, making it a simplified but functional compiler architecture.

## Installation
Download the repository.

Compile the project using any C compiler (GCC recommended).
```bat
Example (Linux / WSL):

gcc main.c -o compiler
./compiler
```

OR on Windows (MinGW):
```bat
gcc main.c -o compiler.exe
compiler.exe
Project Structure
```

Make sure your project contains the following files:
```bat
Models/
compiler_common.h
main.c
input.txt
compilation_logs.txt
Input File
```

Write your Python-like code inside:
`input.txt`

Example:
```bat
x = 5
y = 10
z = x + y
```

## Output
After running the compiler, results and logs will be written to:

`compilation_logs.txt`

This file contains:
Lexical analysis results
Parsing steps
Semantic checks
Final translation Hash Table
Lexical analysis (tokenization)
Syntax parsing
Semantic validation
Translation from Python-like syntax to C
Compilation logging system
Modular compiler architecture
Notes
This project is educational and does not fully support all Python features.
