# f-lang
F programming language (just a learning project)

This project is only for personal purpose, my goal is to learn ASM and a bit about how creating a compiler from scratch.

## Goals
1. Windows platform
1. x86 architecture (x64 architecture eventually)
1. No libc dependency
1. Bootstrap as soon as possible the compiler
1. Make the compiler fast

## TODO 1
1. Generate an exe at PE format with hard coded ASM code
1. Parse and generate ASM code for hello.f program

## TODO 2
As generating a PE file is a bit complicate and took me a lot of time to figure out how all things are working,
I will also work on a c code generator. Doing so, I will be capable to advance on the f language itself, this will
also give the possibility to have an intermediate representation useful for debugging (notably for meta programmation).
After the c code generator I'll may look to add an other step by generating ASM code in nasm file format, the linker
will be the last thing to replace after that.

## Compile
QtCreator (qbs) and Visual Studio solution files are in compiler-cpp folder.
With the Visual Studio solution it is straight forward.
For QtCreator you should set the working directory to the root folder of the git project to be able to run the compiler.

Please notice that for the moment the purpose of the cpp version of the compiler is to build the f-lang version of what
will be the same compiler. So minimal efforts will be done on the cpp version of the compiler.
