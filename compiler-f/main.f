// import f_language_definitions;

/*
myenum :: enum {
	first = 0,
	second
}*/

// foo : Type = u32; // Type is a Type that store a type, this is compile time only. So foo is a compile time variable.

alias DWORD = ui32;
alias HANDLE = §void;

// @TODO I think that it could be
// DWORD :: ui32;
// HANDLE :: §void;
// Alias keyword doesn't seems necessary


// For pointers, we may want to use operators that arent used for something else to simplify the parser implementation.
// § to get a pointer
// @ for unreferencing a pointer
// . accessor operator should be able to automatically unreference the pointer to get the member

// foo : Type = u32; // Type is a Type that store a type, this is compile time only. So foo is a compile time variable.

main :: (arguments : [] string) -> i32
{
/*
	x := 1 + 2 * 3 - 4;
	y := (1 + 2) * 3 - 4;
	z := 1 + 2 * (3 - 4);
	w := (1 + 2) * (3 - 4);
*/
	
/*
	enum_value : myenum;
	
	if enum_value == {
		case first:
		// some code
		// break isn't requested
		case second:
		// some code
	}

*/

	{
		{
			foo : §[]DWORD;
		}
	}

	neasted_function :: ()
	{
	}
	
    message:        string  = "Hello World";
    written_bytes:  DWORD   = 0;
    hstdOut:        HANDLE  = GetStdHandle(STD_OUTPUT_HANDLE);
	strlen:         ui32 = message.length.test;
	//test			f32 = -0.0;

    WriteFile(hstdOut, message.c_string, message.length, §written_bytes, 0);

    // ExitProcess(0);
    /*return 0;*/
}
