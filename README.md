ppjC to FRISC compiler
======================

Programming assignment for the compilers class at my university. This is a compiler from ppjC (a subset of C) to FRISC (an assembly language). The interpreter for FRISC is taken from [here](https://github.com/fer-ppj/FRISCjs).

Architecture
------------

1. Lexer (similar to lex) takes ppjC code as input and outputs a list of lexemes.
2. Parser (similar to yacc) takes the list as input and outputs a syntax tree.
3. Codegen takes the tree as input and outputs FRISC code.

Compilation
-----------
    make

Quick demo
----------
    $ ./run demo/fib.c
    21
    $ ./run demo/pow.c
    1073741824
    $ ./run demo/prime.c
    17
