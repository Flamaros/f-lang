#include "process.hpp"

#include <fstd/platform.hpp>

#include <fstd/core/string_builder.hpp>
#include <fstd/core/unicode.hpp>

import fstd.language.defer;

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		void exit_process(int exit_code)
		{
#if defined(FSTD_OS_WINDOWS)
			ExitProcess(exit_code);
#endif
		}

		bool execute_process(const system::Path& executable_path, const language::string& arguments)
		{
#if defined(FSTD_OS_WINDOWS)
			//language::UTF16LE_string	utf16_executable_path;
			language::UTF16LE_string	utf16_command_line;
			core::String_Builder		string_builder;
			STARTUPINFO					startup_info;
			PROCESS_INFORMATION			process_info;

			defer{
				free_buffers(string_builder);
			};

			ZeroMemory(&startup_info, sizeof(startup_info));
			startup_info.cb = sizeof(startup_info);
			ZeroMemory(&process_info, sizeof(process_info));

			//core::from_utf8_to_utf16LE(to_string(executable_path), utf16_executable_path, true);

			print_to_builder(string_builder, "\"%v\" %s", to_string(executable_path), arguments);
			core::from_utf8_to_utf16LE(to_string(string_builder), utf16_command_line, true);

			// @Warning
			//
			// CreateProcessW is a really bad API, if the executable is specified in lpApplicationName the function
			// will not search the exe elsewhere than in a current folder except if it is an OS command like cmd.exe.
			// If the exe is the first part of lpProcessAttributes it should be placed between " characters to escape
			// spaces that can be in the path which is limited to MAX_PATH (260 characters) even with the unicode version.
			//
			// @TODO consider to do our own search algorithm to put the exe in the lpApplicationName instead.
			// 
			// execute_process can be tricky to write in a portable way with a coherent behavior between OS.
			//
			// Flamaros - 03 may 2020

			if (!CreateProcessW(
				nullptr/* (LPCWSTR)language::to_utf16(utf16_executable_path)*/,	// lpApplicationName,
				(LPWSTR)language::to_utf16(utf16_command_line),		// lpCommandLine,
				NULL,												// lpProcessAttributes,
				NULL,												// lpThreadAttributes,
				FALSE,												// bInheritHandles,
				0,													// dwCreationFlags,
				NULL,												// lpEnvironment,
				NULL,												// lpCurrentDirectory,
				&startup_info,										// lpStartupInfo,
				&process_info										// lpProcessInformation
			)) {
				// @TODO log error here ???
				// Or do we have the let the caller do it?
				return false;
			}

			// Wait until child process exits.
			WaitForSingleObject(process_info.hProcess, INFINITE);

			// Close process and thread handles. 
			CloseHandle(process_info.hProcess);
			CloseHandle(process_info.hThread);
			return true;
#else
#	error
			return false;
#endif
		}
	}
}
