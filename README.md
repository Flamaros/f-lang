# f-lang
F programming language (just a learning project).

This project is only for personal purpose, I am looking for a language that let me do my job with more fun than C++ to do my other personnal projects.
I started this project while waiting to be able to test the Jai language made by Jonathan Blow. So I decided to do my own compiler.
So the syntax and features that I'll try to implement will be close to Jai or other languages I already appreciate like D,...

## Utopia
1. Bootstrap as soon as possible the compiler
1. Multi-platform compiler
1. Fast compiler
1. Resonably fast generated native code
1. No libc dependency

## TODO
1. Lexer
1. Parser
1. C code generator
1. Have enough features to be able to write the compiler in f-lang
1. Have more features

## Long-term TODO 2
1. Generate an exe at PE format with hard coded x86 ASM code
1. Parse and generate x86 ASM code for hello.f program
1. x86 ASM code generator
1. Able to generate debug info (pdb,...)
1. X64 ASM code generator
1. ARM ASM code generator

## Compile/Run
Visual Studio 2019:
1. Open solution that is in compiler-cpp folder
1. Just click on the Run button

QtCreator:
1. Open solution that is in compiler-cpp folder
1. In project configuration. Build&Run section
  1. Set the working directory to the root folder of the git project
  1. Uncheck "Add build library search path to PATH"
  1. For "Execution environment" select "System environment"

Please notice that for the moment the purpose of the cpp version of the compiler is to build a f-lang version that can be used to boostrap it. So minimal efforts will be done on the cpp version of the compiler.
