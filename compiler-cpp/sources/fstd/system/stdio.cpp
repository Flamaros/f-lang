#include "stdio.hpp"

#include <fstd/platform.hpp>

#include <fstd/language/string_view.hpp>

#include <fstd/os/windows/console.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		void print(const language::string_view& string)
		{
#if defined(FSTD_OS_WINDOWS)
			//std::wstring	string(L"\x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd\n");
			DWORD	nb_written = 0;

			// @TODO use the WriteConsoleW
//			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(string), (DWORD)language::get_string_length(string), &nb_written, NULL);
			WriteConsoleA(os::windows::get_std_out_handle(), language::to_utf8(string), (DWORD)language::get_string_length(string), &nb_written, NULL);
#else
#	error
#endif
		}

		void print(const language::string& string)
		{
#if defined(FSTD_OS_WINDOWS)
			//std::wstring	string(L"\x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd\n");
			DWORD	nb_written = 0;

			// @TODO use the WriteConsoleW
//			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(string), (DWORD)language::get_string_length(string), &nb_written, NULL);
			WriteConsoleA(os::windows::get_std_out_handle(), language::to_utf8(string), (DWORD)language::get_string_length(string), &nb_written, NULL);
#else
#	error
#endif
		}
	}
}
