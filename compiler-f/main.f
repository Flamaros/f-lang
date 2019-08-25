import f_language_definitions;

typedef DWORD = ui32;
typedef HANDLE = void*;

fn main :: (arguments : [] string) -> i32
{
    message:        string  = "Hello World";
    written_bytes:  DWORD   = 0;
    hstdOut:        void*   = GetstdHandle(STD_OUTPUT_HANDLE);

    WriteFile(hstdOut, message.c_string, message.length, &bytes, 0);

    // ExitProcess(0);
    return 0;
}
