Python-C-Compiler

A compiler that translates Python to C. Implemented in pure C.

This project demonstrates the core principles of compiler design, including lexical analysis, parsing, semantic checking, and code translation from a high-level language (Python-like syntax) into C code. It focuses on understanding how programming languages are processed and transformed into executable form.


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

