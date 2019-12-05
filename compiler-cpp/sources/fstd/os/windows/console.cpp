#include "console.hpp"

#if defined(PLATFORM_WINDOWS)

#include <Windows.h>

namespace fstd
{
	namespace os
	{
		namespace windows
		{
			static bool	g_allocated_console = false;

			void enable_default_console_configuration()
			{
				HWND	console_window = GetConsoleWindow();
				HANDLE	my_stdout;
				HANDLE	my_stderr;
				HANDLE	my_stdin;
				DWORD	mode;

				if (console_window == NULL) {
					AllocConsole();
					g_allocated_console = true;
				}

				my_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
				my_stderr = GetStdHandle(STD_ERROR_HANDLE);
				my_stdin = GetStdHandle(STD_INPUT_HANDLE);

				// @TODO we should check if utf-8 is supported by OS
				// take a look to EnumSystemCodePages
				SetConsoleOutputCP(CP_UTF8);	// @Warning console will certainly never support full utf-8 encoding, due to the typos, lyrics scripts,...

				mode = GetConsoleMode(my_stdout, &mode);
				mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;	// Enable support of VT100 extensions, this provide colors with \033 escape character
				SetConsoleMode(my_stdout, mode);
			}

			void close_console()
			{
				if (g_allocated_console) {	// We should make a PAUSE before the console will be closed (by the process exit)
					//CreateProcessW(
					//	LPCWSTR               lpApplicationName,
					//	LPWSTR                lpCommandLine,
					//	LPSECURITY_ATTRIBUTES lpProcessAttributes,
					//	LPSECURITY_ATTRIBUTES lpThreadAttributes,
					//	BOOL                  bInheritHandles,
					//	DWORD                 dwCreationFlags,
					//	LPVOID                lpEnvironment,
					//	LPCWSTR               lpCurrentDirectory,
					//	LPSTARTUPINFOW        lpStartupInfo,
					//	LPPROCESS_INFORMATION lpProcessInformation
					//);
				}
			}
		}
	}
}

#endif
