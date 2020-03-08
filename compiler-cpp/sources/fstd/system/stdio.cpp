#include "stdio.hpp"

#include <fstd/platform.hpp>

#include <fstd/language/string_view.hpp>
#include <fstd/core/unicode.hpp>

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
			DWORD	nb_written = 0;

			language::UTF16LE_string	utf16_string;

			core::from_utf8_to_utf16LE(string, utf16_string, false);
			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(utf16_string), (DWORD)language::get_string_size(utf16_string), &nb_written, NULL);
#else
#	error
#endif
		}

		void print(const language::string& string)
		{
#if defined(FSTD_OS_WINDOWS)
			DWORD	nb_written = 0;

			language::UTF16LE_string	utf16_string;

			core::from_utf8_to_utf16LE(string, utf16_string, false);
			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(utf16_string), (DWORD)language::get_string_size(utf16_string), &nb_written, NULL);
#else
#	error
#endif
		}
	}
}
