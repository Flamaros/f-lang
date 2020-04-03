// import f_language_definitions;

/*
myenum :: enum {
	first,
	second
}*/

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
