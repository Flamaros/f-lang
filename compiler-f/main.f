// import f_language_definitions;

/*
myenum :: enum {
	first,
	second
}*/

// foo : Type = u32; // Type is a Type that store a type, this is compile time only. So foo is a compile time variable.

alias DWORD = ui32;
alias HANDLE = void*;

main :: (arguments : [] string) -> i32
{
    message:        string  = "Hello World";
    written_bytes:  DWORD   = 0;
    hstdOut:        void*   = GetstdHandle(STD_OUTPUT_HANDLE);

    WriteFile(hstdOut, message.c_string, message.length, &bytes, 0);

    // ExitProcess(0);
    return 0;
}
