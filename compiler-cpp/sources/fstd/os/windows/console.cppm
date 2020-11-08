export module fstd.os.windows.console;

#include <fstd/platform.hpp>

#if defined(FSTD_OS_WINDOWS)

#	include <Windows.h>

namespace fstd
{
	namespace os
	{
		namespace windows
		{
			export void	enable_default_console_configuration();	// This function activate default configuration for the current console or will open a new one configured correctly (to print in utf-8 with support of VT100 extensions)
			export void	close_console();	// Close the console if where open by the program
			export HANDLE	get_std_out_handle();
		}
	}
}

module: private;

import fstd.system.allocator;

namespace fstd
{
	namespace os
	{
		namespace windows
		{
			static bool	g_allocated_console = false;
			static HANDLE g_stdout;
			static HANDLE g_stderr;
			static HANDLE g_stdin;

			void enable_default_console_configuration()
			{
				HWND	console_window = GetConsoleWindow();
				DWORD	mode;

				if (console_window == NULL) {
					AllocConsole();
					g_allocated_console = true;
				}

				g_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
				g_stderr = GetStdHandle(STD_ERROR_HANDLE);
				g_stdin = GetStdHandle(STD_INPUT_HANDLE);

				// @TODO we should check if utf-8 is supported by OS
				// take a look to EnumSystemCodePages
				SetConsoleOutputCP(CP_UTF8);	// @Warning console will certainly never support full utf-8 encoding, due to the typos, lyrics scripts,...

				mode = GetConsoleMode(g_stdout, &mode);
				mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;	// Enable support of VT100 extensions, this provide colors with \033 escape character
				SetConsoleMode(g_stdout, mode);
			}

			void close_console()
			{
				wchar_t				parameters[32];
				STARTUPINFOW		startup_info;
				PROCESS_INFORMATION	process_info;

				system::zero_memory(&startup_info, sizeof(startup_info));
				startup_info.cb = sizeof(startup_info);
				system::zero_memory(&process_info, sizeof(process_info));

				// @TODO replace that with our string
				system::memory_copy(parameters, (void*)L"cmd.exe /c PAUSE", 16 * sizeof(wchar_t));
				parameters[16] = 0;

				if (g_allocated_console
					|| IsDebuggerPresent()) {	// We should make a PAUSE before the console will be closed (by the process exit)
					CreateProcessW(
						NULL,			// lpApplicationName
						parameters,		// lpCommandLine
						NULL,			// lpProcessAttributes
						NULL,			// lpThreadAttributes
						FALSE,			// bInheritHandles
						0,				// dwCreationFlags
						NULL,			// lpEnvironment
						NULL,			// lpCurrentDirectory,
						&startup_info,	// lpStartupInfo,
						&process_info	// lpProcessInformation
					);

					// Wait until child process exits.
					WaitForSingleObject(process_info.hProcess, INFINITE);

					// Close process and thread handles. 
					CloseHandle(process_info.hProcess);
					CloseHandle(process_info.hThread);
				}
			}

			HANDLE get_std_out_handle() {
				return g_stdout;
			}
		}
	}
}

#endif
