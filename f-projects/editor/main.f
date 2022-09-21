// import f_language_definitions;

// Win32 types
BOOL :: i32; // int
HANDLE :: §void;
PVOID :: §void;
LPCVOID :: §void;
DWORD :: ui32; // unsigned long
LPDWORD :: §DWORD;
ULONG_PTR :: ui64;

// @TODO test circular definition of alias at least
// I should do that with other type definitions
// CIRCULAR_DEF :: CIRCULAR_DEF_ROOT
// CIRCULAR_DEF_ROOT :: CIRCULAR_DEF

/* GetStdHandle: */
STD_INPUT_HANDLE        : DWORD	 = -10;
STD_OUTPUT_HANDLE       : DWORD  = -11;
STD_ERROR_HANDLE        : DWORD  = -12;
// INVALID_HANDLE_VALUE    := cast(HANDLE)-1; // @TODO need to support cast (with the type checker)
ATTACH_PARENT_PROCESS   : DWORD  = -1;

//HANDLE_FLAG_INHERIT             := 0x00000001;
//HANDLE_FLAG_PROTECT_FROM_CLOSE  := 0x00000002;

// -------------------


OVERLAPPED :: struct
{
    Internal : ULONG_PTR;
    InternalHigh : ULONG_PTR;
    DUMMYUNIONNAME : union
	{
        DUMMYSTRUCTNAME : struct
		{
            Offset : DWORD;
            OffsetHigh : DWORD;
        }
        Pointer : PVOID;
    }

    hEvent : HANDLE;
}
LPOVERLAPPED :: §OVERLAPPED;

GetStdHandle :: (nStdHandle : DWORD) -> HANDLE : win32 dll_import("kernel32.dll");
WriteFile :: (hFile : HANDLE,
			  lpBuffer : LPCVOID,
			  nNumberOfBytesToWrite : DWORD,
			  lpNumberOfBytesWritten : LPDWORD,
			  lpOverlapped : LPOVERLAPPED) -> BOOL : win32, dll_import("kernel32.dll");

main :: ()
{
    message:        string  = "Hello World";
    written_bytes:  DWORD; // Should be default initialized
    hstdOut:        HANDLE  = GetStdHandle(STD_OUTPUT_HANDLE);

    WriteFile(hstdOut, message.data, message.size, §written_bytes, 0);

    ExitProcess(0);
}
