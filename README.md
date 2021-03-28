# CRISP

CRISP is an acronym (of course) and stands for **C**ompile and **R**untime
**I**ntro**S**pectable **P**roperties.

CRISP is a small header only library in the spirit of the awesome
[visit\_struct](https://github.com/garbageslam/visit_struct), which aims to
bring type reflection/introspection to the world of C++.

Compared to visit\_struct, CRISP provides type introspection at compile time
**and runtime**. Other notable improvements include e.g. terser syntax, the
capability to introspect **member functions**, and introspection for structs
with **single inheritance**.

## Rational

Writing complex programs in C++ can be a daunting task. Especially, when you
are required to manage (relatively) large codebases on your own, it becomes
necessary to offload as much work as possible to the compiler. Template
meta-programming provides a way to do so, without external tooling and while
maintaining compatibility to common compilers. (Though your sanity is usually
sacrificed in the process ...)

This library was created mostly for the learning experience, but also because I
found existing approaches lacking in some aspects i'd really liked to have
(mainly function and runtime introspection).

## Requirements

CRISP requires at least a C++17 compatible compiler. I've personally only
tested the library with gcc 9.3.0 and clang 10.0.0 and cannot make any
statements about other compilers.

No external dependencies are used. 

A tolerance for C++ templates and absurd error messages is also required.
