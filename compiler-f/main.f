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

GetStdHandle :: (nStdHandle : DWORD) -> HANDLE
	: win32; // @dll(import), @calling_convention(stdcall);
WriteFile :: (hFile : HANDLE,
			  lpBuffer : LPCVOID,
			  nNumberOfBytesToWrite : DWORD,
			  lpNumberOfBytesWritten : LPDWORD,
			  lpOverlapped : LPOVERLAPPED) -> BOOL
	: win32; // @dll(import), @calling_convention(stdcall);

main :: (arguments : [] string) -> i32
{
    message:        string  = "Hello World";
    written_bytes:  DWORD; // Should be default initialized
    hstdOut:        HANDLE  = GetStdHandle(STD_OUTPUT_HANDLE);
	strlen:         ui32    = message.size;
	//test			f32     = -0.0;

//	test_array:     [4]ui32 = message;
	
	x: i32 = 5 * (3 + 4);
	y: i32 = 4 + 3 * 5;
	z: i32 = 5 - 3 + 4;



    WriteFile(hstdOut, message.data, message.size, §written_bytes, 0);

    // ExitProcess(0);
    /*return 0;*/
}
