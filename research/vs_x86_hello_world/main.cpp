#pragma comment(lib, "kernel32.lib")

#include <Windows.h>

int main()
{
	const char* message = "Hello World";
	DWORD written_bytes = 0;

	HANDLE hstdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	WriteFile(hstdOut, message, 11, &written_bytes, 0);

	return 0;
}
